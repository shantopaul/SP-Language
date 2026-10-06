import * as path from 'path';
import * as fs from 'fs';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

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

export function activate(context: vscode.ExtensionContext) {
    startLanguageServer(context);

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
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
