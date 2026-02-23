
import os
import sys
import json
import base64
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ed25519

def sign_report(report_path="report.json"):
    private_key_b64 = os.environ.get("GRADING_PRIVATE_KEY")
    if not private_key_b64:
        print("Error: GRADING_PRIVATE_KEY not set.", file=sys.stderr)
        return False

    try:
        # Load Private Key
        try:
            private_key_bytes = base64.b64decode(private_key_b64)
            if len(private_key_bytes) == 32:
                private_key = ed25519.Ed25519PrivateKey.from_private_bytes(private_key_bytes)
            else:
                private_key = serialization.load_pem_private_key(private_key_bytes, password=None)
        except Exception as e:
            print(f"Error loading key: {e}", file=sys.stderr)
            return False

        # Read or Generate Report
        data = None
        try:
            with open(report_path, "rb") as f:
                report_data = f.read()
            data = json.loads(report_data)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            print(f"Warning: {report_path} invalid/missing ({e}). Generating fallback report.", file=sys.stderr)
            import datetime
            data = {
                "$schema": "http://ntu-os.org/schemas/v1/report",
                "meta": {
                    "generated_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    "grader_image": "unknown",
                    "assignment": "unknown"
                },
                "target_commit": {
                    "sha": "unknown",
                    "author": "unknown",
                    "timestamp": "",
                    "is_late": False
                },
                "grading": {
                    "deadline": "",
                    "is_late": False,
                    "late_days": 0,
                    "penalty_policy": "Execution Failed",
                    "penalty_ratio": 1.0
                },
                "scores": {
                    "raw_total": 0,
                    "max_total": 100,
                    "final_score": 0,
                    "details": [{
                        "test_case": "System Execution",
                        "status": "FAIL",
                        "score": 0,
                        "max_score": 100,
                        "output": "Critical Failure: Grading system failed to produce a valid report. Artifact missing or corrupt."
                    }]
                }
            }

        if "signature" in data:
            del data["signature"]
            
        canonical_bytes = json.dumps(data, sort_keys=True).encode('utf-8')
        
        signature = private_key.sign(canonical_bytes)
        signature_b64 = base64.b64encode(signature).decode('utf-8')
        
        # Write back with signature
        data["signature"] = signature_b64
        with open(report_path, "w") as f:
            json.dump(data, f, indent=2)
            
        print(f"Report signed successfully. Signature: {signature_b64[:10]}...")
        return True

    except Exception as e:
        print(f"Signing failed: {e}", file=sys.stderr)
        return False

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", default="report.json", help="Path to report.json")
    args = parser.parse_args()
    
    if sign_report(args.report):
        sys.exit(0)
    else:
        sys.exit(1)
