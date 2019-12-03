const spawnSync = require("child_process").spawnSync;

const nodeArgs = ["node_modules/doxygen/bin/nodeDoxygen.js", "--docs", "--configPath=Doxyfile"];

const nodeResult = spawnSync("node", nodeArgs, {stdio: ["inherit", "inherit", "pipe"]});
if (nodeResult.stderr.length > 0)
    throw new Error(nodeResult.stderr);

const pythonArgs = ["-m", "doxybook", "-i", "./tmp/xml", "-o", "./gitbook/docs", "-t", "gitbook", "-s", "./SUMMARY.md"];

const pythonResult = spawnSync("python", pythonArgs, {stdio: ["inherit", "inherit", "pipe"]});
if (pythonResult.stderr.length > 0)
    throw new Error(pythonResult.stderr);
