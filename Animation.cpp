#include "Animation.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void Animation::addFrame(const Frame& frame) {
    animationFrames.push_back(frame);
};

// Our card data lives in the XY plane (Z is the "fold" direction), but raylib's
// DrawGrid / default up-vector treat Y as up and XZ as the ground plane. This
// maps data (x, y, z) -> raylib (x, z, -y) so the grid ("floor") lines up with
// the card's own XY plane instead of XZ. It's a proper rotation (det = +1), so
// it doesn't mirror anything, just re-orients which axis is "up".
static inline Vector3 ToRaylib(const Eigen::Vector3f& v) {
    return Vector3{ v.x(), v.z(), -v.y() };
}

void Animation::buildTopology() {
    topology.clear();
    for (Mechanism& mech : card->getMechanisms()) {
        MechanismTopology t;
        t.vertexCount = static_cast<int>(mech.getCurrentPos().size());
        t.creases = mech.getCreases();
        topology.push_back(t);
    }
}

void Animation::animate(float thetaMin, float thetaMax, int windowWidth, int windowHeight) {
    if (!card) {
        std::cerr << "Animation::animate() needs a live PopUpCard (use the Animation(PopUpCard&) constructor).\n";
        return;
    }

    buildTopology();

    InitWindow(windowWidth, windowHeight, "Pop-Up Card Simulator");
    SetTargetFPS(60);

    // The card data is in pattern units (hundreds wide), so size everything off the card's
    // bounding box over the whole animation range instead of hard-coding distances.
    Vector3 boxMin = {  1e30f,  1e30f,  1e30f };
    Vector3 boxMax = { -1e30f, -1e30f, -1e30f };
    for (float t : { thetaMin, thetaMax }) {
        card->actuateWholeCard(-t);
        for (const Eigen::Vector3f& p : card->returnCurrentVertices()) {
            Vector3 r = ToRaylib(p);
            boxMin = Vector3Min(boxMin, r);
            boxMax = Vector3Max(boxMax, r);
        }
    }
    Vector3 sceneCenter = Vector3Scale(Vector3Add(boxMin, boxMax), 0.5f);
    Vector3 boxSize = Vector3Subtract(boxMax, boxMin);
    float sceneSize = fmaxf(fmaxf(boxSize.x, boxSize.y), fmaxf(boxSize.z, 1e-3f));
    float moveSpeed = sceneSize * 0.6f;   // world units per second
    constexpr float rollSpeed = 1.5f;     // radians per second
    constexpr float lookSensitivity = 0.003f;
    rlSetClipPlanes(sceneSize * 0.001, sceneSize * 50.0);

    // Free-fly camera: position plus an orthonormal forward/up pair (so roll is possible).
    Vector3 camPos, camForward, camUp;
    auto resetCamera = [&]() {
        camPos = Vector3Add(sceneCenter, Vector3Scale(Vector3{ 0.0f, 0.6f, 1.0f }, sceneSize * 1.4f));
        camForward = Vector3Normalize(Vector3Subtract(sceneCenter, camPos));
        camUp = Vector3{ 0.0f, 1.0f, 0.0f };
        Vector3 right = Vector3Normalize(Vector3CrossProduct(camForward, camUp));
        camUp = Vector3CrossProduct(right, camForward);
    };
    resetCamera();

    // Grid sized/spaced to the card (it is drawn at the origin, so cover the farthest coordinate).
    float gridSpacing = powf(10.0f, floorf(log10f(sceneSize / 8.0f)));
    float farthest = fmaxf(fmaxf(fabsf(boxMin.x), fabsf(boxMax.x)), fmaxf(fabsf(boxMin.z), fabsf(boxMax.z)));
    int gridSlices = 2 * (int)ceilf(farthest * 1.3f / gridSpacing);
    if (gridSlices > 400) gridSlices = 400;

    Camera3D camera = { 0 };
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float theta = thetaMin;
    static const Color mechColors[] = { BLUE, ORANGE, GREEN, PURPLE, MAROON, DARKBROWN };
    static const Color axisColorU = RED;
    static const Color axisColorV = LIME;
    static const Color axisColorW = (Color){ 30, 120, 255, 255 }; // distinct from crease DARKBLUE

    Rectangle sliderBounds = { 150, (float)windowHeight - 50, windowWidth - 300.0f, 20 };
    bool draggingSlider = false;
    bool showFrames = true;
    bool showVertexIds = false;

    // Draws one axis of a mechanism's u/v/w frame as a shaft + cone arrowhead.
    auto drawFrameAxis = [sceneSize](const Eigen::Vector3f& origin, const Eigen::Vector3f& dir, Color color) {
        const float shaftLen = 0.15f * sceneSize;
        const float headLen = 0.03f * sceneSize;
        Eigen::Vector3f shaftEnd = origin + dir * (shaftLen - headLen);
        Eigen::Vector3f tip = origin + dir * shaftLen;
        Vector3 o  = ToRaylib(origin);
        Vector3 se = ToRaylib(shaftEnd);
        Vector3 tp = ToRaylib(tip);
        DrawCylinderEx(o, se, 0.0045f * sceneSize, 0.0045f * sceneSize, 8, color);  // shaft
        DrawCylinderEx(se, tp, 0.013f * sceneSize, 0.0f, 10, color);  // arrowhead cone
    };

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F)) {
            showFrames = !showFrames;
        }
        if (IsKeyPressed(KEY_V)) {
            showVertexIds = !showVertexIds;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), sliderBounds)) {
            draggingSlider = true;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            draggingSlider = false;
        }

        // --- free-fly camera ---
        float dt = GetFrameTime();
        Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, camUp));

        // mouse drag: look around (pitch about the camera's right axis, yaw about its up axis)
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !draggingSlider) {
            Vector2 mouseDelta = GetMouseDelta();
            float yaw   = -mouseDelta.x * lookSensitivity;
            float pitch = -mouseDelta.y * lookSensitivity;
            camForward = Vector3RotateByAxisAngle(camForward, camUp, yaw);
            camForward = Vector3RotateByAxisAngle(camForward, camRight, pitch);
            camUp      = Vector3RotateByAxisAngle(camUp, camRight, pitch);
            camRight   = Vector3Normalize(Vector3CrossProduct(camForward, camUp));
        }

        // left/right arrows: roll about the view direction
        float rollInput = (IsKeyDown(KEY_RIGHT) ? 1.0f : 0.0f) - (IsKeyDown(KEY_LEFT) ? 1.0f : 0.0f);
        if (rollInput != 0.0f) {
            camUp = Vector3RotateByAxisAngle(camUp, camForward, rollInput * rollSpeed * dt);
            camRight = Vector3Normalize(Vector3CrossProduct(camForward, camUp));
        }

        // up/down arrows: move the camera up/down along its own up axis (view direction unchanged)
        float liftInput = (IsKeyDown(KEY_UP) ? 1.0f : 0.0f) - (IsKeyDown(KEY_DOWN) ? 1.0f : 0.0f);
        camPos = Vector3Add(camPos, Vector3Scale(camUp, liftInput * moveSpeed * dt));

        // WASD: move forward/back and strafe left/right; mouse wheel also moves forward/back
        float forwardInput = (IsKeyDown(KEY_W) ? 1.0f : 0.0f) - (IsKeyDown(KEY_S) ? 1.0f : 0.0f);
        float strafeInput  = (IsKeyDown(KEY_D) ? 1.0f : 0.0f) - (IsKeyDown(KEY_A) ? 1.0f : 0.0f);
        camPos = Vector3Add(camPos, Vector3Scale(camForward, forwardInput * moveSpeed * dt));
        camPos = Vector3Add(camPos, Vector3Scale(camRight, strafeInput * moveSpeed * dt));
        camPos = Vector3Add(camPos, Vector3Scale(camForward, GetMouseWheelMove() * sceneSize * 0.05f));

        if (IsKeyPressed(KEY_R)) resetCamera();

        // re-orthonormalize so drift from repeated rotations can't accumulate
        camForward = Vector3Normalize(camForward);
        camUp = Vector3Normalize(Vector3CrossProduct(camRight, camForward));

        camera.position = camPos;
        camera.target   = Vector3Add(camPos, camForward);
        camera.up       = camUp;

        card->actuateWholeCard(-theta);
        std::vector<Eigen::Vector3f> allPoints = card->returnCurrentVertices();

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                DrawGrid(gridSlices, gridSpacing);

                int offset = 0;
                for (size_t m = 0; m < topology.size(); m++) {
                    int n = topology[m].vertexCount;
                    Color c = mechColors[m % 6];

                    // outline, assuming cyclic connectivity (0->1->...->n-1->0)
                    for (int i = 0; i < n; i++) {
                        Eigen::Vector3f a = allPoints[offset + i];
                        Eigen::Vector3f b = allPoints[offset + (i + 1) % n];
                        DrawLine3D(ToRaylib(a), ToRaylib(b), c);
                    }

                    for (int i = 0; i < n; i++) {
                        Eigen::Vector3f p = allPoints[offset + i];
                        DrawSphere(ToRaylib(p), 0.008f * sceneSize, c);
                    }

                    for (const Crease& cr : topology[m].creases) {
                        Eigen::Vector3f a = allPoints[offset + cr.i];
                        Eigen::Vector3f b = allPoints[offset + cr.j];
                        Color creaseColor = (cr.type == CreaseType::Mountain) ? RED : DARKBLUE;
                        DrawLine3D(ToRaylib(a), ToRaylib(b), creaseColor);
                    }

                    if (showFrames) {
                        const CoordFrame& frame = card->getMechanisms()[m].getCurrentFrame();
                        drawFrameAxis(frame.origin, frame.u, axisColorU);
                        drawFrameAxis(frame.origin, frame.v, axisColorV);
                        drawFrameAxis(frame.origin, frame.w, axisColorW);
                        DrawSphere(ToRaylib(frame.origin), 0.01f * sceneSize, BLACK);
                    }

                    offset += n;
                }
            EndMode3D();

            if (showVertexIds) {
                int labelOffset = 0;
                for (size_t m = 0; m < topology.size(); m++) {
                    int n = topology[m].vertexCount;
                    for (int i = 0; i < n; i++) {
                        Eigen::Vector3f p = allPoints[labelOffset + i];
                        Vector2 screenPos = GetWorldToScreen(ToRaylib(p), camera);
                        DrawText(TextFormat("%d", i), (int)screenPos.x + 6, (int)screenPos.y - 6, 14, BLACK);
                    }
                    labelOffset += n;
                }
            }

            GuiSlider(sliderBounds, "Closed", "Open", &theta, thetaMin, thetaMax);
            DrawText(TextFormat("theta = %.3f rad (%.1f deg)", theta, theta * RAD2DEG),
                     150, windowHeight - 80, 18, DARKGRAY);
            if (showFrames) {
                DrawText("frame:", 150, windowHeight - 105, 16, DARKGRAY);
                DrawText("u", 210, windowHeight - 105, 16, axisColorU);
                DrawText("v", 230, windowHeight - 105, 16, axisColorV);
                DrawText("w", 250, windowHeight - 105, 16, axisColorW);
            }
            DrawText("[F] toggle frames", windowWidth - 180, windowHeight - 105, 16, DARKGRAY);
            DrawText("[V] toggle vertex ids", windowWidth - 180, windowHeight - 85, 16, DARKGRAY);
            DrawText("WASD move, up/down arrows rise/sink, left/right roll, drag to look, [R] reset", 10, 10, 16, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
}