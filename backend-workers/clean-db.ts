import { config } from "dotenv";
import { Pool } from "@neondatabase/serverless";
import { drizzle } from "drizzle-orm/neon-serverless";

// Load environment variables from .dev.vars
config({ path: ".dev.vars" });

/**
 * Truncate all tables (delete all data but keep schema)
 */
const truncateAllTables = async () => {
  if (!process.env.DATABASE_URL) {
    throw new Error("DATABASE_URL not found in .dev.vars");
  }

  console.log("🧹 Starting database cleanup...");
  console.log(
    "📦 Database:",
    process.env.DATABASE_URL.split("@")[1]?.split("?")[0]
  );

  const pool = new Pool({ connectionString: process.env.DATABASE_URL });

  try {
    // Get all table names from information_schema
    const result = await pool.query(`
      SELECT table_name 
      FROM information_schema.tables 
      WHERE table_schema = 'public' 
      AND table_type = 'BASE TABLE'
    `);

    const tables = result.rows.map((row: any) => row.table_name);

    if (tables.length === 0) {
      console.log("⚠️  No tables found to truncate");
      return;
    }

    console.log(`📋 Found ${tables.length} tables: ${tables.join(", ")}`);

    // Truncate each table (Neon doesn't allow session_replication_role)
    for (const table of tables) {
      await pool.query(`TRUNCATE TABLE "${table}" RESTART IDENTITY CASCADE`);
      console.log(`✓ Truncated: ${table}`);
    }

    console.log("✅ Database cleanup complete!");
  } catch (error) {
    console.error("❌ Cleanup failed:", error);
    throw error;
  } finally {
    await pool.end();
  }
};

// Execute cleanup
truncateAllTables()
  .then(() => {
    console.log("👍 All tables truncated - database is ready for fresh start");
    process.exit(0);
  })
  .catch((error) => {
    console.error("💥 Cleanup error:", error);
    process.exit(1);
  });
