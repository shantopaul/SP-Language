import std.process;

fn main() {
    println("--- Standard Library Process Demo ---");

    int myPid = pid();
    println("Current PID: ${myPid}");

    int retCode = exec("cmd.exe /c exit 0");
    println("exec exit code: ${retCode}");

    string output = runCommand("cmd.exe /c echo HelloFromSubprocess");
    println("captured output: ${output}");

    int childPid = spawn("cmd.exe /c exit 0");
    println("spawn child pid: ${childPid}");

    // Static Process class methods
    int staticCode = Process.exec("cmd.exe /c exit 0");
    println("Process.exec: ${staticCode}");

    string staticOut = Process.run("cmd.exe /c echo ProcessStaticRun");
    println("Process.run: ${staticOut}");

    int pPid = Process.pid();
    println("Process.pid: ${pPid}");

    println("All Process tests passed!");
}
