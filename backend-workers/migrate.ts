import { config } from "dotenv";
import { migrate } from "drizzle-orm/neon-serverless/migrator";
import { Pool } from "@neondatabase/serverless";
import { drizzle } from "drizzle-orm/neon-serverless";

// Load environment variables from .dev.vars (Cloudflare Workers format)
config({ path: ".dev.vars" });

const runMigrations = async () => {
  if (!process.env.DATABASE_URL) {
    throw new Error("DATABASE_URL not found in .dev.vars");
  }

  console.log("🔄 Starting database migration...");
  console.log(
    "📦 Database:",
    process.env.DATABASE_URL.split("@")[1]?.split("?")[0]
  );

  const pool = new Pool({ connectionString: process.env.DATABASE_URL });
  const db = drizzle(pool);

  try {
    await migrate(db, { migrationsFolder: "./drizzle" });
    console.log("✅ Migration complete!");
  } catch (error) {
    console.error("❌ Migration failed:", error);
    throw error;
  } finally {
    await pool.end();
  }
};

runMigrations()
  .then(() => {
    console.log("👍 Database is up to date");
    process.exit(0);
  })
  .catch((error) => {
    console.error("💥 Migration error:", error);
    process.exit(1);
  });
