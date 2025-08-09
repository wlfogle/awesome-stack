import React, { useEffect, useRef, useState } from "react";
import { Terminal } from "@xterm/xterm";
import { FitAddon } from "@xterm/addon-fit";
import { invoke } from "@tauri-apps/api/core";
import "@xterm/xterm/css/xterm.css";

interface TerminalComponentProps {
  connection: any;
}

const TerminalComponent: React.FC<TerminalComponentProps> = ({ connection }) => {
  const terminalRef = useRef<HTMLDivElement>(null);
  const terminalInstance = useRef<Terminal | null>(null);
  const fitAddon = useRef<FitAddon | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [currentCommand, setCurrentCommand] = useState('');

  useEffect(() => {
    if (!terminalRef.current) return;

    // Initialize terminal
    const terminal = new Terminal({
      cursorBlink: true,
      fontSize: 14,
      fontFamily: 'Consolas, "Courier New", monospace',
      theme: {
        background: '#1a1a1a',
        foreground: '#ffffff',
        cursor: '#ffffff',
        selection: '#3e3e3e'
      }
    });

    const fit = new FitAddon();
    terminal.loadAddon(fit);
    
    terminalInstance.current = terminal;
    fitAddon.current = fit;

    terminal.open(terminalRef.current);
    fit.fit();

    // Handle terminal input
    terminal.onData(async (data) => {
      if (!isConnected) {
        terminal.write('\r\nNot connected to server\r\n$ ');
        return;
      }

      if (data === '\r') {
        // Enter pressed - execute command
        terminal.write('\r\n');
        
        if (currentCommand.trim()) {
          try {
            const result = await invoke('execute_remote_command', {
              serverId: connection.id,
              command: currentCommand.trim()
            });
            
            terminal.write(result as string);
          } catch (error) {
            terminal.write(`Error: ${error}\r\n`);
          }
        }
        
        terminal.write('$ ');
        setCurrentCommand('');
      } else if (data === '\u007f') {
        // Backspace
        if (currentCommand.length > 0) {
          setCurrentCommand(prev => prev.slice(0, -1));
          terminal.write('\b \b');
        }
      } else if (data === '\u0003') {
        // Ctrl+C
        terminal.write('^C\r\n$ ');
        setCurrentCommand('');
      } else {
        // Regular character
        setCurrentCommand(prev => prev + data);
        terminal.write(data);
      }
    });

    // Connect to server when component mounts
    if (connection) {
      connectToServer(terminal);
    }

    // Handle window resize
    const handleResize = () => {
      fit.fit();
    };
    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      terminal.dispose();
    };
  }, [connection]);

  const connectToServer = async (terminal: Terminal) => {
    if (!connection) return;

    terminal.write('Connecting to Proxmox server...\r\n');
    
    try {
      await invoke('connect_to_proxmox', {
        serverId: connection.id,
        host: connection.host,
        username: connection.username,
        password: connection.password
      });
      
      setIsConnected(true);
      terminal.write(`Connected to ${connection.host}\r\n`);
      terminal.write('$ ');
    } catch (error) {
      terminal.write(`Connection failed: ${error}\r\n`);
      setIsConnected(false);
    }
  };

  return (
    <div className="h-full flex flex-col">
      <div className="bg-gray-800 text-white px-4 py-2 border-b border-gray-700">
        <div className="flex items-center justify-between">
          <h3 className="text-lg font-semibold">Terminal</h3>
          <div className="flex items-center space-x-2">
            <div className={`w-3 h-3 rounded-full ${isConnected ? 'bg-green-500' : 'bg-red-500'}`}></div>
            <span className="text-sm">
              {isConnected ? `Connected to ${connection?.host}` : 'Disconnected'}
            </span>
          </div>
        </div>
      </div>
      <div 
        ref={terminalRef} 
        className="flex-1 bg-gray-900 p-2"
        style={{ minHeight: '400px' }}
      />
    </div>
  );
};

export default TerminalComponent;
