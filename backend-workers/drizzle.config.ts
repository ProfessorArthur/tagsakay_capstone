import { defineConfig } from "drizzle-kit";
import { config } from "dotenv";

// Load .dev.vars for local development
config({ path: ".dev.vars" });

export default defineConfig({
  schema: "./src/db/schema.ts",
  out: "./drizzle",
  dialect: "postgresql",
  dbCredentials: {
    url: process.env.DATABASE_URL!,
  },
  // Add introspection and migration options
  verbose: true,
  strict: true,
});
