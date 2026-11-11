module tests.todo_test;

import std.os;
import std.fs;
import std.io;
import std.json;
import db.schema;
import repositories.category_repo;
import repositories.todo_repo;
import services.auth_service;
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

fn createSampleTodo(
    string userId,
    string title,
    string desc,
    string status,
    string priority,
    string dueDate,
    string catId
) -> TodoResult {
    let jsonStr = "{\"title\":\"" + title + "\",\"description\":\"" + desc
        + "\",\"status\":\"" + status + "\",\"priority\":\"" + priority + "\"";
    if (dueDate.length() > 0) {
        jsonStr = jsonStr + ",\"due_date\":\"" + dueDate + "\"";
    }
    if (catId.length() > 0) {
        jsonStr = jsonStr + ",\"category_id\":\"" + catId + "\"";
    }
    jsonStr = jsonStr + "}";
    let parsed = Json.parse(jsonStr);
    let res = TodoService.create(userId, parsed);
    parsed.destroy();
    return res;
}

fn testTodoCrudAndCompletedAtTransitions() {
    let dbPath = "build/test_todo_crud.db";
    setupTestEnv(dbPath);

    let reg = AuthService.register("Alice", "alice-todo@example.com", "StrongPass#123");
    assertTrue(reg.ok, "Register failed");
    let uid = reg.user.id;
    let cats = CategoryRepository.listByUser(uid);
    assertTrue(cats.length() > 0, "Expected seeded categories");
    let workCat = cats[0];

    // 1. Create todo with category + due_date
    let created = createSampleTodo(
        uid,
        "Prepare SP 4.0 release notes",
        "Summarize compiler and stdlib milestones",
        "todo",
        "urgent",
        "2026-12-15",
        workCat.id
    );
    assertTrue(created.ok && created.statusCode == 201, "Create todo failed");
    assertTrue(created.todo.completedAt == "", "New todo status should not set completedAt");
    assertTrue(created.todo.categoryName == workCat.name, "Expected joined category metadata");

    let todoId = created.todo.id;

    // 2. Mark done -> completedAt populated
    let doneBody = Json.parse("{\"status\":\"done\"}");
    let doneRes = TodoService.update(uid, todoId, doneBody);
    doneBody.destroy();
    assertTrue(doneRes.ok && doneRes.todo.status == "done", "Transition to done failed");
    assertTrue(doneRes.todo.completedAt.length() > 0, "Transition to done must set completedAt");
    let savedCompletedAt = doneRes.todo.completedAt;

    // 3. Archive from done -> completedAt preserved
    let archBody = Json.parse("{\"status\":\"archived\"}");
    let archRes = TodoService.update(uid, todoId, archBody);
    archBody.destroy();
    assertTrue(archRes.ok && archRes.todo.completedAt == savedCompletedAt, "Archive should preserve completedAt");

    // 4. Reopen -> completedAt cleared
    let reopenBody = Json.parse("{\"status\":\"in_progress\"}");
    let reopenRes = TodoService.update(uid, todoId, reopenBody);
    reopenBody.destroy();
    assertTrue(reopenRes.ok && reopenRes.todo.completedAt == "", "Reopen must clear completedAt");

    // 5. Clear nullable category_id & due_date with null JSON values
    let clearBody = Json.parse("{\"category_id\":null,\"due_date\":null}");
    let clearRes = TodoService.update(uid, todoId, clearBody);
    clearBody.destroy();
    assertTrue(clearRes.ok && clearRes.todo.categoryId == "", "category_id null must clear field");
    assertTrue(clearRes.todo.dueDate == "", "due_date null must clear field");

    // 6. Delete todo and confirm 404
    let delRes = TodoService.remove(uid, todoId);
    assertTrue(delRes.ok, "Delete todo failed");
    let afterDel = TodoService.getById(uid, todoId);
    assertTrue(!afterDel.ok && afterDel.statusCode == 404, "Deleted todo lookup must return 404");

    cleanupDbFiles(dbPath);
    println("[PASS] todo CRUD, completed_at transitions, nullable clears, and delete");
}

fn testFilteringPaginationStatsAndBulk() {
    let dbPath = "build/test_todo_query.db";
    setupTestEnv(dbPath);

    let reg = AuthService.register("Alice", "alice-query@example.com", "StrongPass#123");
    assertTrue(reg.ok, "Register failed");
    let uid = reg.user.id;

    let t1 = createSampleTodo(uid, "Argon2 security review", "Check password cost params", "todo", "urgent", "2020-01-01", "");
    let t2 = createSampleTodo(uid, "API endpoint tests", "Write coverage for REST routes", "in_progress", "high", "2099-12-31", "");
    let t3 = createSampleTodo(uid, "Configure WAL backup", "Verify SQLite snapshot", "done", "medium", "", "");
    let t4 = createSampleTodo(uid, "Refactor sidebar badge", "Clean CSS alignment", "todo", "low", "", "");
    assertTrue(t1.ok && t2.ok && t3.ok && t4.ok, "Seed todos failed");

    // Filter active tasks
    let activeFilter = new TodoFilter();
    activeFilter.status = "active";
    let activePage = TodoRepository.listByUser(uid, activeFilter);
    assertTrue(activePage.total == 3, "Expected 3 active todos");

    // Filter overdue tasks
    let overdueFilter = new TodoFilter();
    overdueFilter.dueState = "overdue";
    let overduePage = TodoRepository.listByUser(uid, overdueFilter);
    assertTrue(overduePage.total == 1, "Expected 1 overdue todo");

    // Search keyword
    let searchFilter = new TodoFilter();
    searchFilter.search = "Argon2";
    let searchPage = TodoRepository.listByUser(uid, searchFilter);
    assertTrue(searchPage.total == 1 && searchPage.itemsJson.contains("Argon2 security review"), "Search must match Argon2 todo");

    // Pagination
    let p1Filter = new TodoFilter();
    p1Filter.limit = 2;
    p1Filter.page = 1;
    let p1 = TodoRepository.listByUser(uid, p1Filter);
    assertTrue(p1.total == 4 && p1.totalPages == 2 && p1.hasNext && !p1.hasPrev, "Page 1 pagination metadata mismatch");

    let p2Filter = new TodoFilter();
    p2Filter.limit = 2;
    p2Filter.page = 2;
    let p2 = TodoRepository.listByUser(uid, p2Filter);
    assertTrue(p2.total == 4 && p2.totalPages == 2 && !p2.hasNext && p2.hasPrev, "Page 2 pagination metadata mismatch");

    // Stats check
    let st = TodoRepository.getStats(uid);
    assertTrue(st.total == 4 && st.todoCount == 2 && st.inProgressCount == 1 && st.doneCount == 1, "Status stats mismatch");
    assertTrue(st.overdueCount == 1 && st.urgentCount == 1 && st.highCount == 1, "Overdue/priority stats mismatch");

    // Bulk complete t1 and t2
    let bulkComplete = Json.parse("{\"action\":\"complete\",\"ids\":[\"" + t1.todo.id + "\",\"" + t2.todo.id + "\"]}");
    let bulkRes = TodoService.bulk(uid, bulkComplete);
    bulkComplete.destroy();
    assertTrue(bulkRes.ok && bulkRes.payloadJson.contains("\"affected\":2"), "Bulk complete should affect 2 rows");

    let stAfterBulk = TodoRepository.getStats(uid);
    assertTrue(stAfterBulk.doneCount == 3 && stAfterBulk.overdueCount == 0, "Stats should update after bulk complete");

    // Bulk priority update on t4
    let bulkPrio = Json.parse("{\"action\":\"set_priority\",\"priority\":\"urgent\",\"ids\":[\"" + t4.todo.id + "\"]}");
    let prioRes = TodoService.bulk(uid, bulkPrio);
    bulkPrio.destroy();
    assertTrue(prioRes.ok && prioRes.payloadJson.contains("\"affected\":1"), "Bulk set_priority failed");

    // Bulk delete
    let bulkDel = Json.parse("{\"action\":\"delete\",\"ids\":[\"" + t3.todo.id + "\",\"" + t4.todo.id + "\"]}");
    let delRes = TodoService.bulk(uid, bulkDel);
    bulkDel.destroy();
    assertTrue(delRes.ok && delRes.payloadJson.contains("\"affected\":2"), "Bulk delete failed");
    assertTrue(TodoRepository.getStats(uid).total == 2, "Expected 2 todos remaining after bulk delete");

    cleanupDbFiles(dbPath);
    println("[PASS] todo filtering, search, pagination, stats, and transactional bulk operations");
}

fn testTodoTenantIsolation() {
    let dbPath = "build/test_todo_isolation.db";
    setupTestEnv(dbPath);

    let aliceReg = AuthService.register("Alice", "alice-t@example.com", "StrongPass#123");
    let bobReg = AuthService.register("Bob", "bob-t@example.com", "StrongPass#123");
    assertTrue(aliceReg.ok && bobReg.ok, "Registrations failed");

    let aliceCats = CategoryRepository.listByUser(aliceReg.user.id);
    let aliceCatId = aliceCats[0].id;
    let aliceTodo = createSampleTodo(aliceReg.user.id, "Alice secret roadmap", "", "todo", "high", "", aliceCatId);
    assertTrue(aliceTodo.ok, "Alice todo create failed");

    // Bob cannot view, update, or delete Alice's todo
    let bobGet = TodoService.getById(bobReg.user.id, aliceTodo.todo.id);
    assertTrue(!bobGet.ok && bobGet.statusCode == 403, "Bob get Alice todo must return 403");

    let bobPatchJson = Json.parse("{\"title\":\"Tampered\"}");
    let bobPatch = TodoService.update(bobReg.user.id, aliceTodo.todo.id, bobPatchJson);
    bobPatchJson.destroy();
    assertTrue(!bobPatch.ok && bobPatch.statusCode == 403, "Bob patch Alice todo must return 403");

    let bobDel = TodoService.remove(bobReg.user.id, aliceTodo.todo.id);
    assertTrue(!bobDel.ok && bobDel.statusCode == 403, "Bob delete Alice todo must return 403");

    // Bob cannot attach Alice's category_id to his todo
    let bobBadCat = createSampleTodo(bobReg.user.id, "Bob sneaky todo", "", "todo", "low", "", aliceCatId);
    assertTrue(!bobBadCat.ok && bobBadCat.statusCode == 422, "Cross-tenant category_id must fail with 422");

    // Bob bulk delete cannot affect Alice's todo
    let bobBulk = Json.parse("{\"action\":\"delete\",\"ids\":[\"" + aliceTodo.todo.id + "\"]}");
    let bobBulkRes = TodoService.bulk(bobReg.user.id, bobBulk);
    bobBulk.destroy();
    assertTrue(bobBulkRes.ok && bobBulkRes.payloadJson.contains("\"affected\":0"), "Bob bulk must affect 0 of Alice's rows");
    assertTrue(TodoService.getById(aliceReg.user.id, aliceTodo.todo.id).ok, "Alice todo must stay untouched");

    cleanupDbFiles(dbPath);
    println("[PASS] todo strict multi-tenant isolation");
}

fn main() -> int {
    testTodoCrudAndCompletedAtTransitions();
    testFilteringPaginationStatsAndBulk();
    testTodoTenantIsolation();
    println("All todo tests passed (3/3).");
    return 0;
}
