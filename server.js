const http = require('http');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const PORT = 3000;

const MIME_TYPES = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'text/javascript',
    '.png': 'image/png'
};

const server = http.createServer((req, res) => {
    // API endpoint for running the C backend
    if (req.method === 'POST' && req.url === '/api/run') {
        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });
        
        req.on('end', () => {
            try {
                const data = JSON.parse(body);
                const imgBase64 = data.image.replace(/^data:image\/png;base64,/, "");
                const imgBuffer = Buffer.from(imgBase64, 'base64');
                const imgPath = path.join(__dirname, `temp_${Date.now()}.png`);
                
                // Save the base64 image to a real .png file
                fs.writeFileSync(imgPath, imgBuffer);
                
                let cmd = `./glint ${imgPath}`;
                if (data.mode === 'trace') cmd += ` --trace ${data.limit || 1000}`;
                else if (data.mode === 'dump') cmd += ` --dump`;
                
                // If the Node server is started from Windows, we need to pass the command through WSL
                if (process.platform === 'win32') {
                     const wslPath = imgPath.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (match, p1) => `/mnt/${p1.toLowerCase()}`);
                     const wslDir = __dirname.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (match, p1) => `/mnt/${p1.toLowerCase()}`);
                     cmd = `wsl bash -c "cd '${wslDir}' && ./glint '${wslPath}'`;
                     if (data.mode === 'trace') cmd += ` --trace ${data.limit || 1000}`;
                     else if (data.mode === 'dump') cmd += ` --dump`;
                     else cmd += ` --run`;
                     cmd += `"`;
                }

                // Execute the C backend
                const child = exec(cmd, (error, stdout, stderr) => {
                    if (fs.existsSync(imgPath)) fs.unlinkSync(imgPath); // Cleanup temp file
                    
                    res.writeHead(200, { 'Content-Type': 'application/json' });
                    res.end(JSON.stringify({
                        output: stdout + (stderr ? '\n' + stderr : '')
                    }));
                });

                // Pass stdin input to the process if provided
                if (data.stdin) {
                    child.stdin.write(data.stdin + '\n');
                }
                child.stdin.end();

            } catch (e) {
                res.writeHead(500, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: e.message }));
            }
        });
        return;
    }

    // Serve Static Frontend Files
    let filePath = path.join(__dirname, 'frontend', req.url === '/' ? 'index.html' : req.url);
    const ext = path.extname(filePath);
    
    fs.readFile(filePath, (err, content) => {
        if (err) {
            res.writeHead(404);
            res.end('Not found');
        } else {
            res.writeHead(200, { 'Content-Type': MIME_TYPES[ext] || 'text/plain' });
            res.end(content);
        }
    });
});

server.listen(PORT, () => {
    console.log(`Backend Bridge Server running at http://localhost:${PORT}/`);
    console.log(`Ready to execute C compiler instructions!`);
});
