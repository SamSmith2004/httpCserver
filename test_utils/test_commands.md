1. Basic GET request:
```bash
curl -v http://localhost:8080/
```

2. POST request with data:
```bash
curl -v -X POST -d "name=John&age=30" http://localhost:8080/
```

3. PUT request:
```bash
curl -v -X PUT -d "updated_data" http://localhost:8080/
```

4. DELETE request:
```bash
curl -v -X DELETE http://localhost:8080/delete
```

5. Request with custom headers:
```bash
curl -v -H "Content-Type: application/json" -H "Authorization: Bearer token123" http://localhost:8080/api
```

6. Test concurrent connections:
```bash
for i in {1..10}; do curl http://localhost:8080/ & done
```
