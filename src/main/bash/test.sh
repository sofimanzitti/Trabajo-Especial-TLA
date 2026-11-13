#! /bin/bash

set -u

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
RED='\033[0;31m'
OFF='\033[0m'
STATUS=0
OUTPUT_PATH="$(mktemp -d)"
trap 'rm -rf "$OUTPUT_PATH"' EXIT

echo "Compiler should accept..."
echo ""

for test in $(ls src/test/c/accept/); do
	cat "src/test/c/accept/$test" | ".build/Flex-Bison-Compiler" "$OUTPUT_PATH" >/dev/null 2>&1
	RESULT="$?"
	if [ "$RESULT" == "0" ]; then
		echo -e "    $test, ${GREEN}and it does${OFF} (status $RESULT)"
	else
		STATUS=1
		echo -e "    $test, ${RED}but it rejects${OFF} (status $RESULT)"
	fi
done
echo ""

echo "Compiler should reject..."
echo ""

for test in $(ls src/test/c/reject/); do
	cat "src/test/c/reject/$test" | ".build/Flex-Bison-Compiler" "$OUTPUT_PATH" >/dev/null 2>&1
	RESULT="$?"
	if [ "$RESULT" != "0" ]; then
		echo -e "    $test, ${GREEN}and it does${OFF} (status $RESULT)"
	else
		STATUS=1
		echo -e "    $test, ${RED}but it accepts${OFF} (status $RESULT)"
	fi
done
echo ""

echo "Compiler should generate..."
echo ""

for test in $(ls src/test/c/output/); do
	rm -rf "$OUTPUT_PATH"/*
	cat "src/test/c/output/$test/input" | ".build/Flex-Bison-Compiler" "$OUTPUT_PATH" >/dev/null 2>&1
	if diff -r "src/test/c/output/$test/expected" "$OUTPUT_PATH" >/dev/null 2>&1; then
		echo -e "    $test, ${GREEN}and it does${OFF}"
	else
		STATUS=1
		echo -e "    $test, ${RED}but the artifacts differ${OFF}"
	fi
done
echo ""

echo "All done."
exit $STATUS
