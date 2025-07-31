#!/usr/bin/env python3
"""
Test server that mimics the Android notification server for testing the Qt6 client.
Implements the length-prefixed protocol with handshake, heartbeat, and notification sending.
"""

import socket
import json
import time
import threading
import uuid
from datetime import datetime
import sys
import struct

class NotificationTestServer:
    def __init__(self, host='0.0.0.0', port=8080):
        self.host = host
        self.port = port
        self.socket = None
        self.clients = []
        self.running = False
        
    def start(self):
        """Start the server and listen for connections"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            self.socket.bind((self.host, self.port))
            self.socket.listen(5)
            self.running = True
            
            print(f"🚀 Test server started on {self.host}:{self.port}")
            print("📱 Using length-prefixed protocol format")
            print("=" * 50)
            
            while self.running:
                try:
                    client_socket, client_address = self.socket.accept()
                    print(f"📞 New connection from {client_address}")
                    
                    client_handler = ClientHandler(client_socket, client_address, self)
                    self.clients.append(client_handler)
                    
                    # Start client handler in a separate thread
                    thread = threading.Thread(target=client_handler.handle)
                    thread.daemon = True
                    thread.start()
                    
                except OSError:
                    if self.running:
                        print("❌ Socket error occurred")
                    break
                    
        except Exception as e:
            print(f"❌ Server error: {e}")
        finally:
            self.stop()
    
    def stop(self):
        """Stop the server"""
        self.running = False
        if self.socket:
            self.socket.close()
        for client in self.clients[:]:
            client.disconnect()
        print("🛑 Server stopped")
    
    def remove_client(self, client):
        """Remove a client from the list"""
        if client in self.clients:
            self.clients.remove(client)

class ClientHandler:
    def __init__(self, socket, address, server):
        self.socket = socket
        self.address = address
        self.server = server
        self.is_authenticated = False
        self.device_info = {}
        self.receive_buffer = b''
        
    def handle(self):
        """Handle client communication"""
        try:
            while self.server.running:
                # Receive data
                data = self.socket.recv(4096)
                if not data:
                    print(f"📪 Client {self.address} disconnected")
                    break
                
                print(f"📦 Received {len(data)} bytes from {self.address}")
                    
                self.receive_buffer += data
                self.process_messages()
                
        except Exception as e:
            print(f"❌ Client {self.address} error: {e}")
        finally:
            self.disconnect()
    
    def process_messages(self):
        """Process complete messages from buffer using length prefix"""
        while len(self.receive_buffer) >= 4:
            # Read the 4-byte big-endian length prefix
            length_bytes = self.receive_buffer[:4]
            message_length = struct.unpack('>I', length_bytes)[0]  # Big-endian unsigned int
            
            print(f"📏 Expected message length: {message_length} bytes")
            
            # Check if we have the complete message (prefix + data)
            total_required = 4 + message_length
            if len(self.receive_buffer) < total_required:
                print(f"📦 Incomplete message: need {total_required} bytes, have {len(self.receive_buffer)}")
                break
            
            # Extract the message data (skip the 4-byte prefix)
            message_data = self.receive_buffer[4:4 + message_length]
            self.receive_buffer = self.receive_buffer[total_required:]
            
            if message_data:
                try:
                    message_str = message_data.decode('utf-8')
                    print(f"📨 Received: {message_str}")
                    
                    message = json.loads(message_str)
                    self.handle_message(message)
                    
                except json.JSONDecodeError as e:
                    print(f"❌ JSON decode error: {e}")
                except UnicodeDecodeError as e:
                    print(f"❌ Unicode decode error: {e}")
    
    def handle_message(self, message):
        """Handle a parsed message"""
        msg_type = message.get('type')
        
        if msg_type == 'conn':
            self.handle_connection_request(message)
        elif msg_type == 'ping':
            self.handle_ping(message)
        elif msg_type == 'pong':
            self.handle_pong(message)
        elif msg_type == 'notification_reply':
            self.handle_notification_reply(message)
        elif msg_type == 'notification_action':
            self.handle_notification_action(message)
        elif msg_type == 'notification_dismiss':
            self.handle_notification_dismiss(message)
        else:
            print(f"❓ Unknown message type: {msg_type}")
    
    def handle_connection_request(self, message):
        """Handle connection handshake"""
        payload = message.get('payload', {})
        self.device_info = {
            'device_name': payload.get('device_name', 'Unknown'),
            'platform': payload.get('platform', 'unknown'),
            'version': payload.get('version', '0.0.0'),
            'supports': payload.get('supports', [])
        }
        
        print(f"🤝 Handshake from {self.device_info['device_name']} ({self.device_info['platform']})")
        print(f"   Version: {self.device_info['version']}")
        print(f"   Supports: {', '.join(self.device_info['supports'])}")
        
        # Send ACK response
        ack_message = {
            'type': 'ack',
            'payload': {
                'ref_id': message.get('id'),
                'status': 'ok'
            }
        }
        
        self.send_message(ack_message)
        self.is_authenticated = True
        print(f"✅ {self.device_info['device_name']} authenticated successfully")
        
        # Start sending periodic notifications and pings
        self.start_periodic_tasks()
    
    def handle_ping(self, message):
        """Respond to ping with pong"""
        ping_id = message.get('id')
        print(f"🏓 Ping received from {self.device_info.get('device_name', 'Unknown')}")
        
        pong_message = {
            'type': 'pong',
            'id': ping_id,
            'timestamp': int(time.time()),
            'payload': {
                'device': 'Test-Server'
            }
        }
        
        self.send_message(pong_message)
    
    def handle_pong(self, message):
        """Handle pong response"""
        print(f"🏓 Pong received from {self.device_info.get('device_name', 'Unknown')}")
    
    def handle_notification_reply(self, message):
        """Handle notification reply from client"""
        payload = message.get('payload', {})
        notif_id = payload.get('id')
        action_key = payload.get('key')
        reply_body = payload.get('body')
        
        print(f"💬 Reply to notification {notif_id}: '{reply_body}' (key: {action_key})")
    
    def handle_notification_action(self, message):
        """Handle notification action from client"""
        payload = message.get('payload', {})
        notif_id = payload.get('id')
        action_key = payload.get('key')
        
        print(f"🎬 Action '{action_key}' triggered for notification {notif_id}")
    
    def handle_notification_dismiss(self, message):
        """Handle notification dismiss from client"""
        payload = message.get('payload', {})
        notif_id = payload.get('id')
        
        print(f"❌ Notification {notif_id} dismissed")
    
    def send_message(self, message):
        """Send a message to the client using length prefix"""
        try:
            message_json = json.dumps(message, separators=(',', ':'))
            message_bytes = message_json.encode('utf-8')
            
            # Create 4-byte big-endian length prefix
            length_prefix = struct.pack('>I', len(message_bytes))
            
            # Send length prefix followed by message data
            full_message = length_prefix + message_bytes
            
            print(f"📤 Sending ({len(message_bytes)} bytes): {message_json}")
            self.socket.send(full_message)
            
        except Exception as e:
            print(f"❌ Failed to send message to {self.address}: {e}")
    
    def start_periodic_tasks(self):
        """Start periodic heartbeat and notifications"""
        # Send periodic ping every 30 seconds
        def send_periodic_ping():
            while self.server.running and self.is_authenticated:
                try:
                    ping_message = {
                        'type': 'ping',
                        'id': f'ping-{int(time.time())}',
                        'timestamp': int(time.time()),
                        'payload': {
                            'device': 'Test-Server'
                        }
                    }
                    self.send_message(ping_message)
                    print(f"📤 PING sent with ID: {ping_message['id']}")
                    time.sleep(30)  # Ping every 30 seconds
                except:
                    break
        
        # Send periodic test notifications every 15 seconds
        def send_periodic_notifications():
            notification_count = 0
            sample_notifications = [
                {
                    'id': f'msg_{int(time.time())}_{notification_count}',
                    'title': 'WhatsApp',
                    'body': 'Hey there! How are you doing?',
                    'app': 'WhatsApp',
                    'package': 'com.whatsapp',
                    'timestamp': int(time.time()),
                    'can_reply': True,
                    'actions': [
                        {'title': 'Reply', 'type': 'remote_input', 'key': 'quick_reply'},
                        {'title': 'Mark as Read', 'type': 'action', 'key': 'mark_read'}
                    ]
                },
                {
                    'id': f'email_{int(time.time())}_{notification_count}',
                    'title': 'Gmail',
                    'body': 'You have a new email from your boss',
                    'app': 'Gmail',
                    'package': 'com.google.android.gm',
                    'timestamp': int(time.time()),
                    'can_reply': True,
                    'actions': [
                        {'title': 'Reply', 'type': 'remote_input', 'key': 'email_reply'},
                        {'title': 'Archive', 'type': 'action', 'key': 'archive'},
                        {'title': 'Delete', 'type': 'action', 'key': 'delete'}
                    ]
                }
            ]
            
            while self.server.running and self.is_authenticated:
                try:
                    # Pick a notification
                    notification = sample_notifications[notification_count % len(sample_notifications)].copy()
                    notification['id'] = f'{notification["app"].lower()}_{int(time.time())}_{notification_count}'
                    notification['timestamp'] = int(time.time())
                    
                    notification_message = {
                        'type': 'notification',
                        'id': str(uuid.uuid4()),
                        'timestamp': int(time.time()),
                        'payload': notification
                    }
                    
                    self.send_message(notification_message)
                    notification_count += 1
                    
                    time.sleep(15)  # Send notification every 15 seconds
                except:
                    break
        
        # Start background threads
        ping_thread = threading.Thread(target=send_periodic_ping)
        ping_thread.daemon = True
        ping_thread.start()
        
        notification_thread = threading.Thread(target=send_periodic_notifications)
        notification_thread.daemon = True
        notification_thread.start()
    
    def disconnect(self):
        """Disconnect the client"""
        try:
            self.socket.close()
        except:
            pass
        
        self.server.remove_client(self)
        print(f"🔌 Client {self.address} disconnected")

def main():
    """Main function"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Length-Prefixed Notification Test Server')
    parser.add_argument('--host', default='0.0.0.0', help='Host to bind to (default: 0.0.0.0)')
    parser.add_argument('--port', type=int, default=8080, help='Port to bind to (default: 8080)')
    
    args = parser.parse_args()
    
    server = NotificationTestServer(args.host, args.port)
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\n🛑 Stopping server...")
        server.stop()

if __name__ == '__main__':
    main()
