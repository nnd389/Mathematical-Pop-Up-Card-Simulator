#include "Animation.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void Animation::addFrame(const Frame& frame) {
    animationFrames.push_back(frame);
};

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

    float camYaw = 0.6f;      // orbit angle around target, radians
    float camPitch = 0.35f;   // up/down angle, radians
    float camDistance = 7.6f; // distance from target

    Camera3D camera = { 0 };
    camera.target     = (Vector3){ 0.0f, 0.5f, 0.0f };
    camera.up         = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float theta = thetaMin;
    static const Color mechColors[] = { BLUE, ORANGE, GREEN, PURPLE, MAROON, DARKBROWN };

    Rectangle sliderBounds = { 150, (float)windowHeight - 50, windowWidth - 300.0f, 20 };
    bool draggingSlider = false;

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), sliderBounds)) {
            draggingSlider = true;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            draggingSlider = false;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !draggingSlider) {
            Vector2 mouseDelta = GetMouseDelta();
            camYaw   -= mouseDelta.x * 0.005f;
            camPitch -= mouseDelta.y * 0.005f;
            if (camPitch > 1.5f) camPitch = 1.5f;
            if (camPitch < -1.5f) camPitch = -1.5f;
        }
        camDistance -= GetMouseWheelMove() * 0.5f;
        if (camDistance < 1.0f) camDistance = 1.0f;

        camera.position.x = camera.target.x + camDistance * cosf(camPitch) * sinf(camYaw);
        camera.position.y = camera.target.y + camDistance * sinf(camPitch);
        camera.position.z = camera.target.z + camDistance * cosf(camPitch) * cosf(camYaw);

        card->actuateWholeCard(-theta);
        std::vector<Eigen::Vector3f> allPoints = card->returnCurrentVertices();

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                DrawGrid(10, 0.5f);

                int offset = 0;
                for (size_t m = 0; m < topology.size(); m++) {
                    int n = topology[m].vertexCount;
                    Color c = mechColors[m % 6];

                    // outline, assuming cyclic connectivity (0->1->...->n-1->0)
                    for (int i = 0; i < n; i++) {
                        Eigen::Vector3f a = allPoints[offset + i];
                        Eigen::Vector3f b = allPoints[offset + (i + 1) % n];
                        DrawLine3D((Vector3){ a.x(), a.y(), a.z() },
                                   (Vector3){ b.x(), b.y(), b.z() }, c);
                    }

                    for (int i = 0; i < n; i++) {
                        Eigen::Vector3f p = allPoints[offset + i];
                        DrawSphere((Vector3){ p.x(), p.y(), p.z() }, 0.02f, c);
                    }

                    for (const Crease& cr : topology[m].creases) {
                        Eigen::Vector3f a = allPoints[offset + cr.i];
                        Eigen::Vector3f b = allPoints[offset + cr.j];
                        Color creaseColor = (cr.type == CreaseType::Mountain) ? RED : DARKBLUE;
                        DrawLine3D((Vector3){ a.x(), a.y(), a.z() },
                                   (Vector3){ b.x(), b.y(), b.z() }, creaseColor);
                    }

                    offset += n;
                }
            EndMode3D();

            GuiSlider(sliderBounds, "Closed", "Open", &theta, thetaMin, thetaMax);
            DrawText(TextFormat("theta = %.3f rad (%.1f deg)", theta, theta * RAD2DEG),
                     150, windowHeight - 80, 18, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
}