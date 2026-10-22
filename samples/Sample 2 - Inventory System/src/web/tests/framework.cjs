const { buildSync } = require("esbuild");
const { execFileSync } = require("node:child_process");
const path = require("node:path");
const os = require("node:os");
const output = path.join(os.tmpdir(), "grid-inventory-tests.cjs");
buildSync({
  entryPoints: [
    path.resolve(
      __dirname,
      "../packages/grid-inventory/tests/core.test.ts"
    ),
  ],
  outfile: output,
  bundle: true,
  platform: "node",
  format: "cjs",
});
execFileSync(process.execPath, ["--test", output], { stdio: "inherit" });
