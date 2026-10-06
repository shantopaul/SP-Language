import * as path from 'path';
import * as fs from 'fs';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;
let projectStatusBarItem: vscode.StatusBarItem | undefined;

function findSplsBinary(): string {
    const config = vscode.workspace.getConfiguration('sp');
    const customPath = config.get<string>('languageServer.path');
    if (customPath && fs.existsSync(customPath)) {
        return customPath;
    }

    const defaultWindows = 'C:\\SP-Language\\bin\\spls.exe';
    if (process.platform === 'win32' && fs.existsSync(defaultWindows)) {
        return defaultWindows;
    }

    const defaultUnix = '/usr/local/bin/spls';
    if (fs.existsSync(defaultUnix)) {
        return defaultUnix;
    }

    return process.platform === 'win32' ? 'spls.exe' : 'spls';
}

function findSpdBinary(): string {
    const config = vscode.workspace.getConfiguration('sp');
    const customPath = config.get<string>('debugger.path');
    if (customPath && fs.existsSync(customPath)) {
        return customPath;
    }

    const defaultWindows = 'C:\\SP-Language\\bin\\spd.exe';
    if (process.platform === 'win32' && fs.existsSync(defaultWindows)) {
        return defaultWindows;
    }

    const defaultUnix = '/usr/local/bin/spd';
    if (fs.existsSync(defaultUnix)) {
        return defaultUnix;
    }

    return process.platform === 'win32' ? 'spd.exe' : 'spd';
}

class SpDebugAdapterDescriptorFactory implements vscode.DebugAdapterDescriptorFactory {
    createDebugAdapterDescriptor(session: vscode.DebugSession): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        const spdBinary = findSpdBinary();
        return new vscode.DebugAdapterExecutable(spdBinary, []);
    }
}

function startLanguageServer(context: vscode.ExtensionContext) {
    const splsBinary = findSplsBinary();

    const serverOptions: ServerOptions = {
        command: splsBinary,
        args: []
    };

    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'sp' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.sp')
        }
    };

    client = new LanguageClient(
        'spLanguageServer',
        'SP Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
}

function updateProjectStatusBar() {
    const workspaceFolders = vscode.workspace.workspaceFolders;
    if (!workspaceFolders || workspaceFolders.length === 0) {
        projectStatusBarItem?.hide();
        return;
    }

    const rootPath = workspaceFolders[0].uri.fsPath;
    const tomlPath = path.join(rootPath, 'sp.toml');

    if (fs.existsSync(tomlPath)) {
        if (!projectStatusBarItem) {
            projectStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
            projectStatusBarItem.command = 'sp.buildProject';
        }

        try {
            const content = fs.readFileSync(tomlPath, 'utf8');
            const nameMatch = content.match(/name\s*=\s*"([^"]+)"/);
            const projName = nameMatch ? nameMatch[1] : 'SP Project';
            projectStatusBarItem.text = `$(package) SP: ${projName}`;
            projectStatusBarItem.tooltip = `SP Package Manager Project (${tomlPath}). Click to build with spm.`;
            projectStatusBarItem.show();
        } catch {
            projectStatusBarItem.text = `$(package) SP Project`;
            projectStatusBarItem.show();
        }
    } else {
        projectStatusBarItem?.hide();
    }
}

export function activate(context: vscode.ExtensionContext) {
    startLanguageServer(context);

    // Register DAP Debug Adapter Factory
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory('sp', new SpDebugAdapterDescriptorFactory())
    );

    // Command: Restart Language Server
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.restartServer', async () => {
            if (client) {
                await client.stop();
            }
            startLanguageServer(context);
            vscode.window.showInformationMessage('SP Language Server restarted.');
        })
    );

    // Command: Format Active Document
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.format', async () => {
            await vscode.commands.executeCommand('editor.action.formatDocument');
        })
    );

    // Command: Run Current File
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.run', (uri?: vscode.Uri) => {
            const fileUri = uri || vscode.window.activeTextEditor?.document.uri;
            if (!fileUri) return;

            const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('SP');
            terminal.show();
            terminal.sendText(`spc run "${fileUri.fsPath}"`);
        })
    );

    // Command: Build Current File
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.build', (uri?: vscode.Uri) => {
            const fileUri = uri || vscode.window.activeTextEditor?.document.uri;
            if (!fileUri) return;

            const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('SP');
            terminal.show();
            terminal.sendText(`spc build "${fileUri.fsPath}"`);
        })
    );

    // Command: Run Tests
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.test', () => {
            const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('SP');
            terminal.show();
            terminal.sendText('spc test');
        })
    );

    // Command: Init Project (spm init)
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.initProject', async () => {
            const name = await vscode.window.showInputBox({
                prompt: 'Enter new SP project name',
                placeHolder: 'my_project'
            });
            if (name) {
                const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('SP');
                terminal.show();
                terminal.sendText(`spm init "${name}"`);
            }
        })
    );

    // Command: Build Project (spm build)
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.buildProject', () => {
            const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('SP');
            terminal.show();
            terminal.sendText('spm build');
        })
    );

    // Command: Run Project (spm run)
    context.subscriptions.push(
        vscode.commands.registerCommand('sp.runProject', () => {
            const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('SP');
            terminal.show();
            terminal.sendText('spm run');
        })
    );

    // Project status bar setup
    updateProjectStatusBar();
    const watcher = vscode.workspace.createFileSystemWatcher('**/sp.toml');
    watcher.onDidCreate(() => updateProjectStatusBar());
    watcher.onDidChange(() => updateProjectStatusBar());
    watcher.onDidDelete(() => updateProjectStatusBar());
    context.subscriptions.push(watcher);
}

export function deactivate(): Thenable<void> | undefined {
    projectStatusBarItem?.dispose();
    if (!client) {
        return undefined;
    }
    return client.stop();
}
