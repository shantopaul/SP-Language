module tests.category_test;

import std.os;
import std.fs;
import std.io;
import std.json;
import db.schema;
import services.auth_service;
import services.category_service;
import services.todo_service;

fn cleanupDbFiles(string basePath) {
    if (exists(basePath)) { deleteFile(basePath); }
    if (exists(basePath + "-wal")) { deleteFile(basePath + "-wal"); }
    if (exists(basePath + "-shm")) { deleteFile(basePath + "-shm"); }
}

fn setupTestEnv(string dbPath) {
    cleanupDbFiles(dbPath);
    OS.setEnv("TODOLIST_DB_PATH", dbPath);
    OS.setEnv("JWT_SECRET", "test-only-jwt-secret-key-at-least-32-bytes");
    OS.setEnv("JWT_TTL_SECONDS", "3600");
    assertTrue(DatabaseManager.initSchema(), "DatabaseManager.initSchema failed");
}

fn assertTrue(bool cond, string msg) {
    if (!cond) {
        panic("Assertion failed: " + msg);
    }
}

fn testSeedCrudAndUniqueness() {
    let dbPath = "build/test_category_crud.db";
    setupTestEnv(dbPath);

    let aliceReg = AuthService.register("Alice", "alice-cat@example.com", "StrongPass#123");
    assertTrue(aliceReg.ok, "Alice register failed");
    let aliceId = aliceReg.user.id;

    // 1. Default seeded categories exist (Work, Personal, Errands)
    let listRes = CategoryService.listForUser(aliceId);
    assertTrue(listRes.ok, "Initial category list failed");
    assertTrue(listRes.payloadJson.contains("\"total\":3"), "Expected 3 default categories");
    assertTrue(listRes.payloadJson.contains("\"Work\""), "Expected Work default category");

    // 2. Invalid color rejected with 422
    let badBody = Json.parse("{\"name\":\"Fitness\",\"color\":\"blue\"}");
    let badCreate = CategoryService.create(aliceId, badBody);
    badBody.destroy();
    assertTrue(!badCreate.ok && badCreate.statusCode == 422, "Invalid hex color must return 422");

    // 3. Valid create succeeds
    let createBody = Json.parse("{\"name\":\"Fitness\",\"color\":\"#EF4444\",\"sort_order\":4}");
    let created = CategoryService.create(aliceId, createBody);
    createBody.destroy();
    assertTrue(created.ok && created.statusCode == 201, "Valid category create must return 201");
    let catId = created.category.id;

    // 4. Case-insensitive duplicate rejected with 409
    let dupBody = Json.parse("{\"name\":\"fitness\",\"color\":\"#10B981\"}");
    let dupRes = CategoryService.create(aliceId, dupBody);
    dupBody.destroy();
    assertTrue(!dupRes.ok && dupRes.statusCode == 409, "Duplicate category name must return 409");

    // 5. Partial update works
    let updBody = Json.parse("{\"name\":\"Wellness\",\"color\":\"#10B981\"}");
    let updated = CategoryService.update(aliceId, catId, updBody);
    updBody.destroy();
    assertTrue(updated.ok && updated.category.name == "Wellness", "Category update failed");
    assertTrue(updated.category.color == "#10B981", "Category color did not update");

    // 6. Invalid UUID fails with 400
    let badIdRes = CategoryService.remove(aliceId, "not-a-uuid");
    assertTrue(!badIdRes.ok && badIdRes.statusCode == 400, "Invalid UUID must return 400");

    // 7. Create todo linked to category, check todo_count, then delete category (SET NULL check)
    let todoBody = Json.parse("{\"title\":\"Morning run\",\"category_id\":\"" + catId + "\"}");
    let todoCreated = TodoService.create(aliceId, todoBody);
    todoBody.destroy();
    assertTrue(todoCreated.ok, "Todo creation with category failed");

    let listWithCount = CategoryService.listForUser(aliceId);
    assertTrue(listWithCount.payloadJson.contains("\"todo_count\":1"), "Expected todo_count 1 on category");

    let delRes = CategoryService.remove(aliceId, catId);
    assertTrue(delRes.ok, "Category delete failed");

    // Todo still exists with null category_id (ON DELETE SET NULL)
    let todoAfterCatDel = TodoService.getById(aliceId, todoCreated.todo.id);
    assertTrue(todoAfterCatDel.ok, "Todo should remain after category deletion");
    assertTrue(todoAfterCatDel.todo.categoryId == "", "Todo category_id should become null after category deletion");

    cleanupDbFiles(dbPath);
    println("[PASS] category seed, CRUD, uniqueness, todo_count, and ON DELETE SET NULL");
}

fn testCategoryTenantIsolation() {
    let dbPath = "build/test_category_isolation.db";
    setupTestEnv(dbPath);

    let aliceReg = AuthService.register("Alice", "alice-iso@example.com", "StrongPass#123");
    let bobReg = AuthService.register("Bob", "bob-iso@example.com", "StrongPass#123");
    assertTrue(aliceReg.ok && bobReg.ok, "User registrations failed");

    let aBody = Json.parse("{\"name\":\"Roadmap\",\"color\":\"#6366F1\"}");
    let aliceCat = CategoryService.create(aliceReg.user.id, aBody);
    aBody.destroy();
    assertTrue(aliceCat.ok, "Alice category create failed");

    // Bob can create same category name in his own tenant
    let bBody = Json.parse("{\"name\":\"Roadmap\",\"color\":\"#EC4899\"}");
    let bobCat = CategoryService.create(bobReg.user.id, bBody);
    bBody.destroy();
    assertTrue(bobCat.ok, "Bob should be allowed same category name in isolated tenant");

    // Bob cannot update or delete Alice's category
    let attackBody = Json.parse("{\"name\":\"Hacked\"}");
    let bobUpd = CategoryService.update(bobReg.user.id, aliceCat.category.id, attackBody);
    attackBody.destroy();
    assertTrue(!bobUpd.ok && bobUpd.statusCode == 403, "Cross-tenant category update must return 403");

    let bobDel = CategoryService.remove(bobReg.user.id, aliceCat.category.id);
    assertTrue(!bobDel.ok && bobDel.statusCode == 403, "Cross-tenant category delete must return 403");

    cleanupDbFiles(dbPath);
    println("[PASS] category multi-tenant isolation");
}

fn main() -> int {
    testSeedCrudAndUniqueness();
    testCategoryTenantIsolation();
    println("All category tests passed (2/2).");
    return 0;
}
