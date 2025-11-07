import { Hono } from "hono";
import { eq } from "drizzle-orm";
import { authMiddleware, requireRole } from "../middleware/auth";
import { users, rfids } from "../db/schema";
import { hashPassword } from "../lib/auth";
import type { Database } from "../db";

type Env = {
  Bindings: {
    DATABASE_URL: string;
    JWT_SECRET: string;
    SESSION_SECRET: string;
  };
  Variables: {
    db: Database;
    user: {
      id: number;
      email: string;
      role: "admin" | "superadmin" | "driver";
    };
  };
};

const app = new Hono<Env>();

/**
 * GET /api/users
 * Get all users (admin only)
 */
app.get("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  try {
    const db = c.get("db");

    // Get all users with their RFID tags
    const usersList = await db
      .select({
        id: users.id,
        name: users.name,
        email: users.email,
        role: users.role,
        isActive: users.isActive,
        rfidTag: users.rfidTag,
        createdAt: users.createdAt,
        updatedAt: users.updatedAt,
      })
      .from(users);

    // Get RFID tags for each user
    const usersWithRfids = await Promise.all(
      usersList.map(async (user) => {
        const userRfids = await db
          .select()
          .from(rfids)
          .where(eq(rfids.userId, user.id));

        return {
          ...user,
          rfidTags: userRfids,
        };
      })
    );

    return c.json({
      success: true,
      count: usersWithRfids.length,
      data: usersWithRfids,
    });
  } catch (error) {
    console.error("Error getting users:", error);
    return c.json(
      {
        success: false,
        message: "Server error",
        error: error instanceof Error ? error.message : "Unknown error",
      },
      500
    );
  }
});

/**
 * GET /api/users/:id
 * Get a specific user by ID
 */
app.get("/:id", authMiddleware, async (c) => {
  try {
    const db = c.get("db");
    const userId = parseInt(c.req.param("id"));

    if (isNaN(userId)) {
      return c.json(
        {
          success: false,
          message: "Invalid user ID",
        },
        400
      );
    }

    // Get user (excluding password)
    const [user] = await db
      .select({
        id: users.id,
        name: users.name,
        email: users.email,
        role: users.role,
        isActive: users.isActive,
        rfidTag: users.rfidTag,
        createdAt: users.createdAt,
        updatedAt: users.updatedAt,
      })
      .from(users)
      .where(eq(users.id, userId));

    if (!user) {
      return c.json(
        {
          success: false,
          message: "User not found",
        },
        404
      );
    }

    // Get user's RFID tags
    const userRfids = await db
      .select()
      .from(rfids)
      .where(eq(rfids.userId, userId));

    return c.json({
      success: true,
      data: {
        ...user,
        rfidTags: userRfids,
      },
    });
  } catch (error) {
    console.error("Error getting user:", error);
    return c.json(
      {
        success: false,
        message: "Server error",
        error: error instanceof Error ? error.message : "Unknown error",
      },
      500
    );
  }
});

/**
 * POST /api/users
 * Create a new user (admin only)
 */
app.post("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  try {
    const db = c.get("db");
    const body = await c.req.json();
    const { name, email, password, role, isActive } = body;

    // Validate required fields
    if (!name || !email || !password) {
      return c.json(
        {
          success: false,
          message: "Name, email, and password are required",
        },
        400
      );
    }

    // Check if user already exists
    const [existingUser] = await db
      .select()
      .from(users)
      .where(eq(users.email, email));

    if (existingUser) {
      return c.json(
        {
          success: false,
          message: "User with this email already exists",
        },
        400
      );
    }

    // Hash the password
    const hashedPassword = await hashPassword(password);

    // Create new user
    const [newUser] = await db
      .insert(users)
      .values({
        name,
        email,
        password: hashedPassword,
        role: role || "driver",
        isActive: isActive !== undefined ? isActive : true,
      })
      .returning({
        id: users.id,
        name: users.name,
        email: users.email,
        role: users.role,
        isActive: users.isActive,
        createdAt: users.createdAt,
      });

    return c.json(
      {
        success: true,
        message: "User created successfully",
        data: newUser,
      },
      201
    );
  } catch (error) {
    console.error("Error creating user:", error);
    return c.json(
      {
        success: false,
        message: "Server error",
        error: error instanceof Error ? error.message : "Unknown error",
      },
      500
    );
  }
});

/**
 * PUT /api/users/:id
 * Update a user (admin only)
 */
app.put(
  "/:id",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const userId = parseInt(c.req.param("id"));
      const body = await c.req.json();
      const { name, email, password, role, isActive } = body;

      if (isNaN(userId)) {
        return c.json(
          {
            success: false,
            message: "Invalid user ID",
          },
          400
        );
      }

      // Check if user exists
      const [existingUser] = await db
        .select()
        .from(users)
        .where(eq(users.id, userId));

      if (!existingUser) {
        return c.json(
          {
            success: false,
            message: "User not found",
          },
          404
        );
      }

      // Prepare update data
      const updateData: any = {
        updatedAt: new Date(),
      };

      if (name) updateData.name = name;
      if (email) updateData.email = email;
      if (password) {
        updateData.password = await hashPassword(password);
      }
      if (role) updateData.role = role;
      if (isActive !== undefined) updateData.isActive = isActive;

      // Update user
      const [updatedUser] = await db
        .update(users)
        .set(updateData)
        .where(eq(users.id, userId))
        .returning({
          id: users.id,
          name: users.name,
          email: users.email,
          role: users.role,
          isActive: users.isActive,
          updatedAt: users.updatedAt,
        });

      return c.json({
        success: true,
        message: "User updated successfully",
        data: updatedUser,
      });
    } catch (error) {
      console.error("Error updating user:", error);
      return c.json(
        {
          success: false,
          message: "Server error",
          error: error instanceof Error ? error.message : "Unknown error",
        },
        500
      );
    }
  }
);

/**
 * DELETE /api/users/:id
 * Delete a user (admin only)
 */
app.delete(
  "/:id",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const userId = parseInt(c.req.param("id"));
      const currentUser = c.get("user");

      if (isNaN(userId)) {
        return c.json(
          {
            success: false,
            message: "Invalid user ID",
          },
          400
        );
      }

      // Prevent deleting self
      if (userId === currentUser.id) {
        return c.json(
          {
            success: false,
            message: "Cannot delete your own account",
          },
          400
        );
      }

      // Check if user exists
      const [existingUser] = await db
        .select()
        .from(users)
        .where(eq(users.id, userId));

      if (!existingUser) {
        return c.json(
          {
            success: false,
            message: "User not found",
          },
          404
        );
      }

      // Delete user
      await db.delete(users).where(eq(users.id, userId));

      return c.json({
        success: true,
        message: "User deleted successfully",
      });
    } catch (error) {
      console.error("Error deleting user:", error);
      return c.json(
        {
          success: false,
          message: "Server error",
          error: error instanceof Error ? error.message : "Unknown error",
        },
        500
      );
    }
  }
);

export default app;
