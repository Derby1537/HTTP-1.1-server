# HTTP Server 
This is a simple HTTP/1.1 server made in c using threads

## How to get started
First of we need to clone the repository
```
git clone https://github.com/Derby1537/HTTP-1.1-server.git
cd HTTP-1.1-server
```
Now we need to compile the server.c file
```
gcc -Wall -o server server.c -lpthread
```
And we've setted everything up
## Starting the server
Now that we've compiled it we can start the server
```
./server 8080
```
You can substitute 8080 with any port that you want. It will be the port to connect to the server

## Visiting the server
Go to your browser and type [localhost:8080](localhost:8080)

If you've changed the port you should put the port number instead of 8080

Now you can visit your own http server made in c
