// Requires node.js. Run with `node logserver.js`.

const net = require("net");
net.createServer((socket) => {
    console.log("new connection from", `${socket.remoteAddress}:${socket.remotePort}`);
    socket
        .on("data", (data) => process.stdout.write(data.toString("utf-8")))
        .on("close", (error) => console.log("disconnect", error))
        .on("error", () => console.log("error"));
}).listen(3080, () => { console.log("listening"); });
