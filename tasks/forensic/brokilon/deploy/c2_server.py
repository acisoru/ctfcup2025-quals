#!/usr/bin/env python3
"""
C2 Server implementation with RC4 encryption
Communicates via stdin/stdout
"""

import sys
import json
import base64
import hashlib
import os
import logging

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class RC4:
    """RC4 cipher implementation"""

    def __init__(self, key):
        self.key = key.encode() if isinstance(key, str) else key
        self.sbox = self._init_sbox()

    def _init_sbox(self):
        """Initialize S-box"""
        sbox = list(range(256))
        j = 0
        for i in range(256):
            j = (j + sbox[i] + self.key[i % len(self.key)]) % 256
            sbox[i], sbox[j] = sbox[j], sbox[i]
        return sbox

    def encrypt(self, data):
        """Encrypt data using RC4"""
        data_bytes = data.encode() if isinstance(data, str) else data
        sbox = self.sbox.copy()
        result = []
        i = j = 0

        for byte in data_bytes:
            i = (i + 1) % 256
            j = (j + sbox[i]) % 256
            sbox[i], sbox[j] = sbox[j], sbox[i]
            k = sbox[(sbox[i] + sbox[j]) % 256]
            result.append(byte ^ k)

        return bytes(result)

    def decrypt(self, data):
        """Decrypt data using RC4 (same as encrypt for RC4)"""
        return self.encrypt(data)

class C2Server:
    """C2 Server implementation"""

    def __init__(self):
        # Static RC4 key
        self.rc4_key = "std::vector_key"
        self.rc4 = RC4(self.rc4_key)

        # Hardcoded auth hash (SHA256 of "admin123")
        self.auth_hash = "d91bf246270157aa9f369093a5e3f29b"

        # File storage directory
        self.storage_dir = "./files"
        os.makedirs(self.storage_dir, exist_ok=True)

        self.authenticated = False

    def encrypt_packet(self, packet):
        """Encrypt and encode packet"""
        json_data = json.dumps(packet)
        encrypted = self.rc4.encrypt(json_data)
        encoded = base64.b64encode(encrypted).decode()
        return encoded

    def decrypt_packet(self, encoded_data):
        """Decode and decrypt packet"""
        try:
            encrypted = base64.b64decode(encoded_data)
            decrypted = self.rc4.decrypt(encrypted)
            packet = json.loads(decrypted.decode())
            return packet
        except Exception as e:
            logger.error(f"Failed to decrypt packet: {e}")
            return None

    def handle_auth(self, packet):
        """Handle authentication command"""
        if 'hash' not in packet:
            return {'status': 'error', 'message': 'Hash required'}

        provided_hash = packet['hash']
        if provided_hash == self.auth_hash:
            self.authenticated = True
            return {'status': 'success', 'message': 'Authentication successful'}
        else:
            return {'status': 'error', 'message': 'Authentication failed'}

    def handle_upload(self, packet):
        """Handle file upload command"""
        return {'status': 'error', 'message': 'Not supported required'}
        
        if not self.authenticated:
            return {'status': 'error', 'message': 'Authentication required'}

        if 'filename' not in packet or 'data' not in packet:
            return {'status': 'error', 'message': 'Filename and data required'}

        try:
            filename = packet['filename']
            file_data = base64.b64decode(packet['data'])
            file_path = os.path.join(self.storage_dir, filename)

            with open(file_path, 'wb') as f:
                f.write(file_data)

            return {'status': 'success', 'message': f'File {filename} uploaded successfully'}
        except Exception as e:
            logger.error(f"Upload error: {e}")
            return {'status': 'error', 'message': f'Upload failed: {str(e)}'}

    def handle_download(self, packet):
        """Handle file download command"""
        if not self.authenticated:
            return {'status': 'error', 'message': 'Authentication required'}

        if 'filename' not in packet:
            return {'status': 'error', 'message': 'Filename required'}

        try:
            filename = packet['filename']
            filename = filename.replace('.', '')
            filename = filename.replace('/', '')
            
            file_path = os.path.join(self.storage_dir, filename)

            if not os.path.exists(file_path):
                return {'status': 'error', 'message': f'File {filename} not found'}

            with open(file_path, 'rb') as f:
                file_data = f.read()

            encoded_data = base64.b64encode(file_data).decode()
            return {'status': 'success', 'filename': filename, 'data': encoded_data}
        except Exception as e:
            logger.error(f"Download error: {e}")
            return {'status': 'error', 'message': f'Download failed: {str(e)}'}

    def handle_command(self, packet):
        """Handle incoming command"""
        if not packet or 'cmd' not in packet:
            return {'status': 'error', 'message': 'Invalid packet format'}

        cmd = packet['cmd']
        logger.info(f"Processing command: {cmd}")

        if cmd == 'auth':
            return self.handle_auth(packet)
        elif cmd == 'upload':
            return self.handle_upload(packet)
        elif cmd == 'download':
            return self.handle_download(packet)
        else:
            return {'status': 'error', 'message': f'Unknown command: {cmd}'}

    def run(self):
        """Main server loop"""
        logger.info("C2 Server started")

        while True:
            try:
                # Read encrypted packet from stdin
                line = sys.stdin.readline().strip()
                if not line:
                    break

                logger.debug(f"Received: {line}")

                # Decrypt packet
                packet = self.decrypt_packet(line)
                if packet is None:
                    response = {'status': 'error', 'message': 'Failed to decrypt packet'}
                else:
                    # Handle command
                    response = self.handle_command(packet)

                # Send encrypted response
                encrypted_response = self.encrypt_packet(response)
                print(encrypted_response)
                sys.stdout.flush()

            except KeyboardInterrupt:
                logger.info("Server stopped by user")
                break
            except Exception as e:
                logger.error(f"Error in main loop: {e}")
                response = {'status': 'error', 'message': 'Internal server error'}
                encrypted_response = self.encrypt_packet(response)
                print(encrypted_response)
                sys.stdout.flush()

if __name__ == "__main__":
    server = C2Server()
    server.run()