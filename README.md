
# quiz-socket
Project ini menggunakan socket untuk komunikasi antara client dan server.

## Catatan
Sebelum menggunakan program ini, konfigurasi terlebih dahulu port yang digunakan. Cari baris kode dibawah ini dalam program ```server.c``` dan ```client.c``` lalu ubah ip yang ingin digunakan. Jika ingin menjalankan di ```localhost``` ubah ip menjadi ```127.0.0.1```. Jika ingin menjalankannya di server, ubah ip dengan ip server.
```
char  *ip  =  "128.199.244.249";  // inisialisasi ip yang digunakan
```

## Cara menggunakan program
#### Cara compile server
gcc -pthread server.c -o server

#### Cara compile client
gcc -pthread client.c -o client

#### Cara eksekusi program server
./server < Port >

contoh : ./server 9999

#### Cara eksekusi program client
./client < Port >

contoh : ./client 9999
