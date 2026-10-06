import std.fs;

fn main() {
    println("--- std.fs: File System Operations ---");

    string filePath = "temp_demo.txt";
    string dirPath = "temp_demo_dir";

    // 1. Write text
    bool writeOk = writeText(filePath, "Hello from SP File System!");
    println("writeText ok: ${writeOk}");
    println("exists: ${exists(filePath)}");
    println("isFile: ${isFile(filePath)}");

    // 2. Read text
    string content = readText(filePath);
    println("readText: ${content}");

    // 3. Append text
    bool appendOk = appendText(filePath, " Additional data.");
    println("appendText ok: ${appendOk}");
    string updatedContent = readText(filePath);
    println("updated readText: ${updatedContent}");

    // 4. Directory operations
    bool mkdirOk = createDir(dirPath);
    println("createDir ok: ${mkdirOk}");
    println("dir exists: ${exists(dirPath)}");
    println("isDir: ${isDir(dirPath)}");

    string subFilePath = "temp_demo_dir/nested.txt";
    writeText(subFilePath, "Nested content");
    string[] files = listDir(dirPath);
    println("listDir count: ${files.length()}");
    println("first file: ${files[0]}");

    // 5. Static FS / File class usage
    println("FS.exists: ${FS.exists(filePath)}");
    println("File.isFile: ${File.isFile(filePath)}");

    // 6. Cleanup
    bool delFileOk = delete(filePath);
    bool delDirOk = delete(dirPath);
    println("delete file ok: ${delFileOk}");
    println("delete dir ok: ${delDirOk}");
    println("file exists after delete: ${exists(filePath)}");
    println("dir exists after delete: ${exists(dirPath)}");

    println("std.fs complete!");
}
