import requests
import json

BASE_URL: str = "http://localhost:8080"

def test_requests():
    # Test POST with text/plain
    print("Testing POST with text/plain")
    response = requests.post(
        f"{BASE_URL}/test",
        data="This is a plain text POST",
        headers={"Content-Type": "text/plain"}
    )
    print(f"Status: {response.status_code}")
    print(f"Response: {response.text}\n")

    # Test POST with application/json
    print("Testing POST with application/json")
    response = requests.post(
        f"{BASE_URL}/test",
        json={"message": "This is a JSON POST"},
        headers={"Content-Type": "application/json"}
    )
    print(f"Status: {response.status_code}")
    print(f"Response: {response.text}\n")

    # Test GET
    print("Testing GET")
    response = requests.get(f"{BASE_URL}/test")
    print(f"Status: {response.status_code}")
    print(f"Response: {response.text}\n")

    # Test PUT with text/plain
    print("Testing PUT with text/plain")
    response = requests.put(
        f"{BASE_URL}/test",
        data="This is a plain text PUT",
        headers={"Content-Type": "text/plain"}
    )
    print(f"Status: {response.status_code}")
    print(f"Response: {response.text}\n")

    # Test PUT with application/json
    print("Testing PUT with application/json")
    response = requests.put(
        f"{BASE_URL}/test",
        json={"message": "This is a JSON PUT"},
        headers={"Content-Type": "application/json"}
    )
    print(f"Status: {response.status_code}")
    print(f"Response: {response.text}\n")

    # Test DELETE
    print("Testing DELETE")
    response = requests.delete(f"{BASE_URL}/test")
    print(f"Status: {response.status_code}")
    print(f"Response: {response.text}\n")

    # Test GET after DELETE
    print("Testing GET after DELETE")
    response = requests.get(f"{BASE_URL}/test")
    print(f"Status: {response.status_code}")
    print(f"Response: {response.text}\n")

if __name__ == "__main__":
    test_requests()
