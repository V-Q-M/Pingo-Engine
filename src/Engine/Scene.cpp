#include "Scene.h"

#include <utility>

#include "Engine.h"

Scene::Scene(Engine &engine, SceneOptions options)
    : engine(engine),
      options(std::move(options)) {
}

const SceneOptions &Scene::Options() const {
    return options;
}

void Scene::Enter() {
}

void Scene::Exit() {
}

void Scene::Update(float) {
}

void Scene::UpdateDevelopment(float) {
}

void Scene::DrawScreen() {
}

bool Scene::DrawsScreenLayer() const {
    return false;
}

bool Scene::OnEscape() {
    return false;
}

bool Scene::CapturesKeyboard() const {
    return false;
}

bool Scene::UsesArrowKeys() const {
    return false;
}

Rectangle Scene::WorldArea() const {
    return {0.0f, 0.0f, 0.0f, 0.0f};
}

void Scene::Draw() {
}

void Scene::DrawUI() {
}

void Scene::AddCommands(ConsoleCommands &) {
}

void Scene::LeaveFromPauseMenu() {
    engine.Quit();
}
