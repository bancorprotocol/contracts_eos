const fs        = require("fs");
const spawnSync = require("child_process").spawnSync;

const nodeArgs   = ["node_modules/doxygen/bin/nodeDoxygen.js", "--docs", "--configPath=Doxyfile"];
const pythonArgs = ["-m", "doxybook", "-i=tmp/xml", "-o=gitbook/docs", "-t=gitbook", "-s=SUMMARY.md"];

const nodeResult = spawnSync("node", nodeArgs, {stdio: ["inherit", "inherit", "pipe"]});
if (nodeResult.stderr.length > 0)
    throw new Error(nodeResult.stderr);

const pythonResult = spawnSync("python3", pythonArgs, {stdio: ["inherit", "inherit", "pipe"]});
if (pythonResult.stderr.length > 0)
    throw new Error(pythonResult.stderr);

for (const fileName of fs.readdirSync("gitbook/docs"))
    if (fileName.startsWith("_") || fileName.startsWith("dir_"))
        fs.unlinkSync("gitbook/docs/" + fileName);

fs.unlinkSync("gitbook/docs/pages.md");
fs.unlinkSync("gitbook/docs/files.md");
fs.unlinkSync("gitbook/docs/macros.md");
fs.unlinkSync("gitbook/docs/variables.md");
fs.unlinkSync("gitbook/docs/functions.md");
fs.unlinkSync("gitbook/docs/annotated.md");
fs.unlinkSync("gitbook/docs/classes.md");
fs.unlinkSync("gitbook/docs/hierarchy.md");
fs.unlinkSync("gitbook/docs/class_members.md");
fs.unlinkSync("gitbook/docs/class_member_functions.md");
fs.unlinkSync("gitbook/docs/class_member_variables.md");
fs.unlinkSync("gitbook/docs/class_member_typedefs.md");
fs.unlinkSync("gitbook/docs/class_member_enums.md");
fs.unlinkSync("gitbook/docs/namespaces.md");
fs.unlinkSync("gitbook/docs/namespacestd.md");
fs.unlinkSync("gitbook/docs/namespaceeosio.md");
fs.unlinkSync("gitbook/docs/namespace_members.md");
fs.unlinkSync("gitbook/docs/namespace_member_functions.md");
fs.unlinkSync("gitbook/docs/namespace_member_variables.md");
fs.unlinkSync("gitbook/docs/namespace_member_typedefs.md");
fs.unlinkSync("gitbook/docs/namespace_member_enums.md");
