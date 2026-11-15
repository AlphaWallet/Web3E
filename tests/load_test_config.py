"""
Load test credentials from test_config.ini (untracked file)

This script reads Wi-Fi and RPC credentials from test_config.ini
and injects them as build flags. The config file is never committed.
"""
Import("env")
import os

config_file = os.path.join(env.get("PROJECT_DIR"), "test_config.ini")

if os.path.exists(config_file):
    print(f"Loading test credentials from {config_file}")
    
    try:
        with open(config_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#') and not line.startswith(';'):
                    if '=' in line:
                        key, value = line.split('=', 1)
                        key = key.strip()
                        value = value.strip().strip('"').strip("'")
                        
                        # Inject as build flag
                        valid_keys = [
                            'WIFI_SSID', 'WIFI_PASS',
                            'TEST_PRIVATE_KEY', 'TEST_TO_ADDRESS',
                            'TEST_TOKEN_CONTRACT', 'ERC721_CONTRACT'
                        ]
                        if key in valid_keys:
                            env.Append(CPPDEFINES=[(key, env.StringifyMacro(value))])
                            print(f"  - Loaded {key}")
    except Exception as e:
        print(f"Warning: Could not parse {config_file}: {e}")
else:
    print("No test_config.ini found - network tests will use empty credentials")
    print("To run network tests, copy test_config.ini.example to test_config.ini")
