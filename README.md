
# quiz-socket
Project ini dibuat oleh kelompok Spike untuk memenuhi tugas besar sistem operasi. Kami menggunakan socket untuk komunikasi antara client dan server.

## Anggota Kelompok

| Nama        | NIM |
| ----------- | ----------- |
| Mochamad Mufid Abiyyu      | 1907996       |
| Muhammad Rayhan Fadillah   |  1907998        |
| Rizal Amri Hidayat      | 1903163       |
| Zuhal Robbani   | 1904178        |
| Sudirman Nur Putra      | 1900457       |


## Catatan
Sebelum menggunakan program ini, konfigurasi terlebih dahulu ip yang digunakan. Cari baris kode ini `char  *ip  =  "128.199.244.249";  // inisialisasi ip yang digunakan ` dalam program ```server.c``` dan ```client.c``` lalu ubah ip yang ingin digunakan. Jika ingin menjalankan di ```localhost``` ubah ip menjadi ```127.0.0.1```. Jika ingin menjalankannya di server, ubah ip dengan ip server yang anda miliki.



## Cara menjalankan program
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

##### Port yang digunakan oleh `server` dan `client` harus sama.
