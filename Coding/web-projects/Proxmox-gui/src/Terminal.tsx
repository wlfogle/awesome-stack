import React, { useEffect, useRef, useState } from 'react';
import { Terminal } from '@xterm/xterm';
import { FitAddon } from '@xterm/addon-fit';
import { invoke } from '@tauri-apps/api/core';
import '@xterm/xterm/css/xterm.css';

interface TerminalProps {
  host: string;
  username: string;
  password: string;
}

const TerminalComponent: React.FC<TerminalProps> = ({ host, username, password }) => {
  const terminalRef = useRef<HTMLDivElement>(null);
  const [terminal, setTerminal] = useState<Terminal | null>(null);
  const [fitAddon, setFitAddon] = useState<FitAddon | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [currentCommand, setCurrentCommand] = useState('');
  const [isConnecting, setIsConnecting] = useState(false);

  useEffect(() => {
    if (terminalRef.current) {
      const term = new Terminal({
        cursorBlink: true,
        fontFamily: 'Monaco, Menlo, "Ubuntu Mono", monospace',
        fontSize: 14,
        theme: {
          background: '#1a1a1a',
          foreground: '#ffffff',
          cursor: '#ffffff',
          cursorAccent: '#000000',
          selectionBackground: '#3366cc',
        },
      });

      const addon = new FitAddon();
      term.loadAddon(addon);
      term.open(terminalRef.current);
      addon.fit();

      setTerminal(term);
      setFitAddon(addon);

      // Welcome message
      term.writeln('\x1b[1;32mProxmox Terminal\x1b[0m');
      term.writeln('Connecting to ' + host + '...');
      term.write('\r\n$ ');

      // Handle terminal input
      term.onData((data) => {
        if (!isConnected) return;

        if (data === '\r') {
          // Enter key pressed
          if (currentCommand.trim()) {
            executeCommand(currentCommand.trim());
            setCurrentCommand('');
          } else {
            term.write('\r\n$ ');
          }
        } else if (data === '\u007f') {
          // Backspace
          if (currentCommand.length > 0) {
            setCurrentCommand(currentCommand.slice(0, -1));
            term.write('\b \b');
          }
        } else if (data >= ' ') {
          // Printable character
          setCurrentCommand(currentCommand + data);
          term.write(data);
        }
      });

      // Handle window resize
      const handleResize = () => {
        if (addon) {
          addon.fit();
        }
      };

      window.addEventListener('resize', handleResize);

      return () => {
        window.removeEventListener('resize', handleResize);
        term.dispose();
      };
    }
  }, []);

  const connectToServer = async () => {
    if (!terminal || isConnecting) return;

    setIsConnecting(true);
    try {
      await invoke('connect_to_proxmox', {
        host,
        username,
        password,
      });

      setIsConnected(true);
      terminal.writeln('\x1b[1;32mConnected successfully!\x1b[0m');
      terminal.write('$ ');
    } catch (error) {
      terminal.writeln('\x1b[1;31mConnection failed: ' + error + '\x1b[0m');
      terminal.write('\r\n$ ');
    } finally {
      setIsConnecting(false);
    }
  };

  const executeCommand = async (command: string) => {
    if (!terminal || !isConnected) return;

    terminal.write('\r\n');

    try {
      const output = await invoke<string>('execute_remote_command', {
        host,
        command,
      });

      // Write command output
      if (output) {
        const lines = output.split('\n');
        lines.forEach((line: string) => {
          terminal.writeln(line);
        });
      }
    } catch (error) {
      terminal.writeln('\x1b[1;31mError: ' + error + '\x1b[0m');
    }

    terminal.write('$ ');
  };

  useEffect(() => {
    if (terminal && !isConnected && !isConnecting) {
      connectToServer();
    }
  }, [terminal, isConnected, isConnecting]);

  useEffect(() => {
    if (fitAddon) {
      const timer = setTimeout(() => {
        fitAddon.fit();
      }, 100);
      return () => clearTimeout(timer);
    }
  }, [fitAddon]);

  return (
    <div className="h-full w-full bg-gray-900 p-4">
      <div className="h-full w-full bg-black rounded-lg overflow-hidden border border-gray-700">
        <div className="bg-gray-800 px-4 py-2 text-sm text-gray-300 border-b border-gray-700">
          <div className="flex items-center justify-between">
            <span>Terminal - {host}</span>
            <div className="flex items-center space-x-2">
              <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-500' : isConnecting ? 'bg-yellow-500' : 'bg-red-500'}`}></div>
              <span className="text-xs">
                {isConnected ? 'Connected' : isConnecting ? 'Connecting...' : 'Disconnected'}
              </span>
            </div>
          </div>
        </div>
        <div 
          ref={terminalRef} 
          className="h-[calc(100%-3rem)] w-full p-2"
          style={{ minHeight: '400px' }}
        />
      </div>
    </div>
  );
};

export default TerminalComponent;
