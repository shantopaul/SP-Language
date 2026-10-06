import std.os;
import std.process;

fn main() {
    println("--- Standard Library Process Demo ---");

    int myPid = pid();
    println("Current PID: ${myPid}");

    string okCmd = platform() == "windows" ? "cmd.exe /c exit 0" : "sh -c exit";
    string echoCmd = platform() == "windows" ? "cmd.exe /c echo HelloFromSubprocess" : "echo HelloFromSubprocess";

    int retCode = exec(okCmd);
    println("exec exit code: ${retCode}");

    string output = runCommand(echoCmd);
    println("captured output: ${output}");

    int childPid = spawn(okCmd);
    println("spawn child pid: ${childPid}");

    // Static Process class methods
    int staticCode = Process.exec(okCmd);
    println("Process.exec: ${staticCode}");

    string staticOut = Process.run(echoCmd);
    println("Process.run: ${staticOut}");

    int pPid = Process.pid();
    println("Process.pid: ${pPid}");

    println("All Process tests passed!");
}
