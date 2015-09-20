echo "Run 1: bufSize = 1 byte" > stats.txt
./a.out -b 1 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 2: bufSize = 2 bytes" >> stats.txt
./a.out -b 2 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 3: bufSize = 4 bytes" >> stats.txt
./a.out -b 4 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 4: bufSize = 8 bytes" >> stats.txt
./a.out -b 8 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 5: bufSize = 16 bytes" >> stats.txt
./a.out -b 16 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 6: bufSize = 32 bytes" >> stats.txt
./a.out -b 32 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 7: bufSize = 64 bytes" >> stats.txt
./a.out -b 64 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 8: bufSize = 128 bytes" >> stats.txt
./a.out -b 128 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 9: bufSize = 256 bytes" >> stats.txt
./a.out -b 256 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 10: bufSize = 512 bytes" >> stats.txt
./a.out -b 512 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 11: bufSize = 1024 bytes" >> stats.txt
./a.out -b 1024 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 12: bufSize = 2048 bytes" >> stats.txt
./a.out -b 2048 -o out test1.txt >> stats.txt
echo "" >> stats.txt

echo "Run 13: bufSize = 4096 bytes" >> stats.txt
./a.out -b 4096 -o out test1.txt >> stats.txt
