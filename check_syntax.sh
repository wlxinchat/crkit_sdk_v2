#!/bin/bash

echo "=== TC-001: Static Code Analysis ==="
echo ""

ERRORS=0
CHECKED=0

# Check all header files
for file in src/utils/*.h src/core/*.h src/backend/inference_backend.h include/*.h; do
  if [ -f "$file" ]; then
    echo -n "Checking $file ... "
    CHECKED=$((CHECKED + 1))
    if g++ -std=c++11 -fsyntax-only -I./include -I./src "$file" 2>/tmp/error.log; then
      echo "✓ OK"
    else
      echo "✗ FAILED"
      cat /tmp/error.log | head -5
      ERRORS=$((ERRORS + 1))
    fi
  fi
done

echo ""
echo "=== Summary ==="
echo "Files checked: $CHECKED"
echo "Errors: $ERRORS"
echo ""

if [ $ERRORS -eq 0 ]; then
  echo "✓ TC-001 PASSED: All header files syntax correct"
  exit 0
else
  echo "✗ TC-001 FAILED: $ERRORS files have syntax errors"
  exit 1
fi
