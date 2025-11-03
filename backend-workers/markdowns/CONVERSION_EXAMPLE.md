# Controller Migration Example

## Converting userController.js to Workers

This example shows **exactly** how to convert one complete controller from Express/Sequelize to Workers/Drizzle.

---

## ❌ BEFORE (Express + Sequelize)

**File:** `backend/src/controllers/userController.js`

```javascript
import { User, Rfid } from "../models/index.js";
import logger from "../config/logger.js";
import bcrypt from "bcryptjs";

// Get all users
export const getUsers = async (req, res) => {
  try {
    const users = await User.findAll({
      attributes: { exclude: ["password"] },
      include: [
        {
          model: Rfid,
          as: "ownedRfid",
          attributes: ["tagId", "isActive"],
        },
      ],
    });

    res.json({
      success: true,
      data: users,
    });
  } catch (error) {
    logger.error("Error fetching users:", error);
    res.status(500).json({
      success: false,
      message: "Failed to fetch users",
      error: error.message,
    });
  }
};

// Get user by ID
export const getUserById = async (req, res) => {
  try {
    const { id } = req.params;

    const user = await User.findByPk(id, {
      attributes: { exclude: ["password"] },
      include: [
        {
          model: Rfid,
          as: "ownedRfid",
        },
      ],
    });

    if (!user) {
      return res.status(404).json({
        success: false,
        message: "User not found",
      });
    }

    res.json({
      success: true,
      data: user,
    });
  } catch (error) {
    logger.error(`Error fetching user ${req.params.id}:`, error);
    res.status(500).json({
      success: false,
      message: "Failed to fetch user",
      error: error.message,
    });
  }
};

// Create user
export const createUser = async (req, res) => {
  try {
    const { name, email, password, role, rfidTag } = req.body;

    // Validation
    if (!name || !email || !password) {
      return res.status(400).json({
        success: false,
        message: "Name, email, and password are required",
      });
    }

    // Check if email exists
    const existingUser = await User.findOne({ where: { email } });
    if (existingUser) {
      return res.status(409).json({
        success: false,
        message: "Email already exists",
      });
    }

    // Hash password
    const hashedPassword = await bcrypt.hash(password, 10);

    // Create user
    const user = await User.create({
      name,
      email,
      password: hashedPassword,
      role: role || "driver",
      rfidTag: rfidTag || null,
    });

    // Remove password from response
    const userResponse = user.toJSON();
    delete userResponse.password;

    logger.info(`User created: ${user.email} by admin ${req.user.id}`);

    res.status(201).json({
      success: true,
      message: "User created successfully",
      data: userResponse,
    });
  } catch (error) {
    logger.error("Error creating user:", error);
    res.status(500).json({
      success: false,
      message: "Failed to create user",
      error: error.message,
    });
  }
};

// Update user
export const updateUser = async (req, res) => {
  try {
    const { id } = req.params;
    const { name, email, role, isActive, rfidTag } = req.body;

    const user = await User.findByPk(id);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: "User not found",
      });
    }

    // Update fields
    if (name) user.name = name;
    if (email) user.email = email;
    if (role) user.role = role;
    if (typeof isActive === "boolean") user.isActive = isActive;
    if (rfidTag !== undefined) user.rfidTag = rfidTag;

    await user.save();

    const userResponse = user.toJSON();
    delete userResponse.password;

    logger.info(`User updated: ${user.email} by admin ${req.user.id}`);

    res.json({
      success: true,
      message: "User updated successfully",
      data: userResponse,
    });
  } catch (error) {
    logger.error(`Error updating user ${req.params.id}:`, error);
    res.status(500).json({
      success: false,
      message: "Failed to update user",
      error: error.message,
    });
  }
};

// Delete user
export const deleteUser = async (req, res) => {
  try {
    const { id } = req.params;

    const user = await User.findByPk(id);

    if (!user) {
      return res.status(404).json({
        success: false,
        message: "User not found",
      });
    }

    await user.destroy();

    logger.info(`User deleted: ${user.email} by admin ${req.user.id}`);

    res.json({
      success: true,
      message: "User deleted successfully",
    });
  } catch (error) {
    logger.error(`Error deleting user ${req.params.id}:`, error);
    res.status(500).json({
      success: false,
      message: "Failed to delete user",
      error: error.message,
    });
  }
};
```

---

## ✅ AFTER (Cloudflare Workers + Drizzle)

**File:** `backend-workers/src/routes/user.ts`

```typescript
import { Hono } from "hono";
import { authMiddleware, requireRole } from "../middleware/auth";
import { users, rfids, type NewUser } from "../db/schema";
import { eq, and } from "drizzle-orm";
import { hashPassword } from "../lib/auth";

const app = new Hono();

// GET /api/users - Get all users (admin only)
app.get("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  try {
    const db = c.get("db");

    // Sequelize: User.findAll({ include: [{ model: Rfid, as: 'ownedRfid' }] })
    // Drizzle equivalent with join:
    const userList = await db
      .select({
        id: users.id,
        name: users.name,
        email: users.email,
        role: users.role,
        isActive: users.isActive,
        rfidTag: users.rfidTag,
        createdAt: users.createdAt,
        updatedAt: users.updatedAt,
        // Include related RFID if exists
        ownedRfid: {
          tagId: rfids.tagId,
          isActive: rfids.isActive,
        },
      })
      .from(users)
      .leftJoin(rfids, eq(users.rfidTag, rfids.tagId));

    return c.json({
      success: true,
      data: userList,
    });
  } catch (error: any) {
    console.error("Error fetching users:", error);
    return c.json(
      {
        success: false,
        message: "Failed to fetch users",
        error: error.message,
      },
      500
    );
  }
});

// GET /api/users/:id - Get user by ID
app.get(
  "/:id",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const id = parseInt(c.req.param("id"));

      if (isNaN(id)) {
        return c.json(
          {
            success: false,
            message: "Invalid user ID",
          },
          400
        );
      }

      // Sequelize: User.findByPk(id, { include: [...] })
      // Drizzle equivalent:
      const [result] = await db
        .select({
          id: users.id,
          name: users.name,
          email: users.email,
          role: users.role,
          isActive: users.isActive,
          rfidTag: users.rfidTag,
          createdAt: users.createdAt,
          updatedAt: users.updatedAt,
          ownedRfid: {
            tagId: rfids.tagId,
            isActive: rfids.isActive,
          },
        })
        .from(users)
        .leftJoin(rfids, eq(users.rfidTag, rfids.tagId))
        .where(eq(users.id, id))
        .limit(1);

      if (!result) {
        return c.json(
          {
            success: false,
            message: "User not found",
          },
          404
        );
      }

      return c.json({
        success: true,
        data: result,
      });
    } catch (error: any) {
      console.error(`Error fetching user ${c.req.param("id")}:`, error);
      return c.json(
        {
          success: false,
          message: "Failed to fetch user",
          error: error.message,
        },
        500
      );
    }
  }
);

// POST /api/users - Create user (admin only)
app.post("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  try {
    const db = c.get("db");
    const currentUser = c.get("user");
    const { name, email, password, role, rfidTag } = await c.req.json();

    // Validation
    if (!name || !email || !password) {
      return c.json(
        {
          success: false,
          message: "Name, email, and password are required",
        },
        400
      );
    }

    // Check if email exists
    // Sequelize: User.findOne({ where: { email } })
    // Drizzle equivalent:
    const [existingUser] = await db
      .select()
      .from(users)
      .where(eq(users.email, email))
      .limit(1);

    if (existingUser) {
      return c.json(
        {
          success: false,
          message: "Email already exists",
        },
        409
      );
    }

    // Hash password
    const hashedPassword = await hashPassword(password);

    // Create user
    // Sequelize: User.create({ ... })
    // Drizzle equivalent:
    const newUser: NewUser = {
      name,
      email,
      password: hashedPassword,
      role: role || "driver",
      rfidTag: rfidTag || null,
    };

    const [createdUser] = await db.insert(users).values(newUser).returning({
      id: users.id,
      name: users.name,
      email: users.email,
      role: users.role,
      isActive: users.isActive,
      rfidTag: users.rfidTag,
      createdAt: users.createdAt,
    });

    console.log(
      `User created: ${createdUser.email} by admin ${currentUser.id}`
    );

    return c.json(
      {
        success: true,
        message: "User created successfully",
        data: createdUser,
      },
      201
    );
  } catch (error: any) {
    console.error("Error creating user:", error);
    return c.json(
      {
        success: false,
        message: "Failed to create user",
        error: error.message,
      },
      500
    );
  }
});

// PUT /api/users/:id - Update user (admin only)
app.put(
  "/:id",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const currentUser = c.get("user");
      const id = parseInt(c.req.param("id"));
      const { name, email, role, isActive, rfidTag } = await c.req.json();

      if (isNaN(id)) {
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
        .where(eq(users.id, id))
        .limit(1);

      if (!existingUser) {
        return c.json(
          {
            success: false,
            message: "User not found",
          },
          404
        );
      }

      // Build update object (only include provided fields)
      const updateData: Partial<typeof users.$inferInsert> = {};
      if (name !== undefined) updateData.name = name;
      if (email !== undefined) updateData.email = email;
      if (role !== undefined) updateData.role = role;
      if (isActive !== undefined) updateData.isActive = isActive;
      if (rfidTag !== undefined) updateData.rfidTag = rfidTag;
      updateData.updatedAt = new Date();

      // Update user
      // Sequelize: user.save()
      // Drizzle equivalent:
      const [updatedUser] = await db
        .update(users)
        .set(updateData)
        .where(eq(users.id, id))
        .returning({
          id: users.id,
          name: users.name,
          email: users.email,
          role: users.role,
          isActive: users.isActive,
          rfidTag: users.rfidTag,
          updatedAt: users.updatedAt,
        });

      console.log(
        `User updated: ${updatedUser.email} by admin ${currentUser.id}`
      );

      return c.json({
        success: true,
        message: "User updated successfully",
        data: updatedUser,
      });
    } catch (error: any) {
      console.error(`Error updating user ${c.req.param("id")}:`, error);
      return c.json(
        {
          success: false,
          message: "Failed to update user",
          error: error.message,
        },
        500
      );
    }
  }
);

// DELETE /api/users/:id - Delete user (admin only)
app.delete(
  "/:id",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const currentUser = c.get("user");
      const id = parseInt(c.req.param("id"));

      if (isNaN(id)) {
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
        .where(eq(users.id, id))
        .limit(1);

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
      // Sequelize: user.destroy()
      // Drizzle equivalent:
      await db.delete(users).where(eq(users.id, id));

      console.log(
        `User deleted: ${existingUser.email} by admin ${currentUser.id}`
      );

      return c.json({
        success: true,
        message: "User deleted successfully",
      });
    } catch (error: any) {
      console.error(`Error deleting user ${c.req.param("id")}:`, error);
      return c.json(
        {
          success: false,
          message: "Failed to delete user",
          error: error.message,
        },
        500
      );
    }
  }
);

export default app;
```

---

## 🔑 Key Conversion Patterns

### 1. **Route Definition**

```javascript
// Express
app.get('/users', authMiddleware, getUsers);

// Hono
app.get('/', authMiddleware, async (c) => { ... });
```

### 2. **Request/Response**

```javascript
// Express
const { id } = req.params;
const data = req.body;
res.json({ success: true });

// Hono
const id = c.req.param("id");
const data = await c.req.json();
return c.json({ success: true });
```

### 3. **Database Queries**

```javascript
// Sequelize findAll with include
const users = await User.findAll({
  include: [{ model: Rfid, as: "ownedRfid" }],
});

// Drizzle select with leftJoin
const userList = await db
  .select()
  .from(users)
  .leftJoin(rfids, eq(users.rfidTag, rfids.tagId));
```

### 4. **Finding by Primary Key**

```javascript
// Sequelize
const user = await User.findByPk(id);

// Drizzle
const [user] = await db.select().from(users).where(eq(users.id, id)).limit(1);
```

### 5. **Creating Records**

```javascript
// Sequelize
const user = await User.create({ name, email });

// Drizzle
const [user] = await db.insert(users).values({ name, email }).returning();
```

### 6. **Updating Records**

```javascript
// Sequelize
await User.update({ name }, { where: { id } });

// Drizzle
await db.update(users).set({ name }).where(eq(users.id, id));
```

### 7. **Deleting Records**

```javascript
// Sequelize
await user.destroy();

// Drizzle
await db.delete(users).where(eq(users.id, id));
```

---

## 📝 Testing the Converted Routes

```bash
# Start local development
npm run dev

# Test GET all users (need JWT token first)
curl http://localhost:8787/api/users \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"

# Test GET user by ID
curl http://localhost:8787/api/users/1 \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"

# Test POST create user
curl -X POST http://localhost:8787/api/users \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "John Doe",
    "email": "john@example.com",
    "password": "password123",
    "role": "driver"
  }'

# Test PUT update user
curl -X PUT http://localhost:8787/api/users/1 \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Jane Doe",
    "isActive": true
  }'

# Test DELETE user
curl -X DELETE http://localhost:8787/api/users/1 \
  -H "Authorization: Bearer YOUR_JWT_TOKEN"
```

---

## ✅ Checklist for Converting Each Controller

When converting a controller, make sure you:

- [ ] Import correct dependencies (`Hono`, `authMiddleware`, schema types)
- [ ] Convert `req/res` to Hono's `c` (context) object
- [ ] Convert Sequelize queries to Drizzle queries
- [ ] Handle validation errors properly
- [ ] Return proper status codes (200, 201, 400, 404, 500)
- [ ] Add authentication middleware where needed
- [ ] Add role-based access control where needed
- [ ] Test each endpoint locally before moving to next
- [ ] Keep error messages consistent with original

---

**Use this as a template for converting your remaining controllers!**
