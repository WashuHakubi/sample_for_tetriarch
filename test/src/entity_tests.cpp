/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <wut/entity.h>

using namespace wut;

TEST_CASE("Can create root object") {
  auto root = Entity::createRoot();
  REQUIRE(root != nullptr);
  REQUIRE(root->root() == root);
  REQUIRE(root->enabledInTree());
  REQUIRE(root->enabledSelf());
  REQUIRE(root->parent() == nullptr);

  // We should not be able to change the root object's enabled state.
  root->setEnabled(false);
  REQUIRE(root->enabledInTree());
  REQUIRE(root->enabledSelf());
}

TEST_CASE("Only rooted objects can be enabled in tree") {
  auto root = Entity::createRoot();
  auto go = Entity::create("go");
  auto childGo = Entity::create("childGo", go);
  auto disabledChildGo = Entity::create("disabledChildGo", go);
  disabledChildGo->setEnabled(false);

  REQUIRE(go->enabledSelf());
  REQUIRE(go->enabledInTree() == false);
  REQUIRE(go->parent() == nullptr);

  REQUIRE(childGo->enabledSelf());
  REQUIRE(childGo->enabledInTree() == false);
  REQUIRE(childGo->parent() == go);

  REQUIRE(disabledChildGo->enabledSelf() == false);
  REQUIRE(disabledChildGo->enabledInTree() == false);
  REQUIRE(disabledChildGo->parent() == go);

  // Adding an entity to the root should enable it in the tree if it is enabled, along with all enabled children.
  go->setParent(root);
  REQUIRE(go->parent() == root);
  REQUIRE(go->enabledInTree());
  REQUIRE(childGo->enabledInTree());
  // Disabled child should not be enabled in the tree.
  REQUIRE(disabledChildGo->enabledInTree() == false);

  // Enabling a child while the parent is enabled in the tree also enables the child in the tree.
  disabledChildGo->setEnabled(true);
  REQUIRE(disabledChildGo->enabledInTree());
  disabledChildGo->setEnabled(false);

  // Disable parent
  go->setEnabled(false);
  REQUIRE(go->enabledSelf() == false);
  REQUIRE(go->enabledInTree() == false);

  // Child object should still be enabled, but not in the tree
  REQUIRE(childGo->enabledSelf());
  REQUIRE(childGo->enabledInTree() == false);

  // Disabled child should still be disabled.
  REQUIRE(disabledChildGo->enabledSelf() == false);
  REQUIRE(disabledChildGo->enabledInTree() == false);
}

TEST_CASE("Destroying entities") {
  auto go = Entity::create("go");
  auto childGo = Entity::create("childGo", go);
  auto destroyChildGo = Entity::create("destroyChildGo", go);

  REQUIRE(go->enabledSelf());
  REQUIRE(childGo->enabledSelf());
  REQUIRE(destroyChildGo->enabledSelf());

  // Only we should be referencing this.
  REQUIRE(go.use_count() == 1);
  // go should reference the children.
  REQUIRE(childGo.use_count() == 2);
  REQUIRE(destroyChildGo.use_count() == 2);

  // Schedule this for destruction.
  destroyChildGo->destroy();
  // But we should still be referenced.
  REQUIRE(destroyChildGo.use_count() == 2);
  REQUIRE(destroyChildGo->enabledSelf() == false);

  // This should have no effect.
  destroyChildGo->postUpdate();
  REQUIRE(destroyChildGo.use_count() == 2);

  // Run the postUpdate on the parent.
  go->postUpdate();
  // We should no longer be referenced by go.
  REQUIRE(destroyChildGo.use_count() == 1);
  // But this child should be.
  REQUIRE(childGo.use_count() == 2);
}
