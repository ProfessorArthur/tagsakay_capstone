import {
  pgTable,
  varchar,
  integer,
  boolean,
  timestamp,
  uuid,
  json,
  pgEnum,
  text,
} from "drizzle-orm/pg-core";
import { relations } from "drizzle-orm";

// Enums
export const roleEnum = pgEnum("role", ["admin", "superadmin", "driver"]);
export const eventTypeEnum = pgEnum("event_type", ["entry", "exit", "unknown"]);
export const scanStatusEnum = pgEnum("scan_status", [
  "success",
  "failed",
  "unauthorized",
]);

// Users table
export const users = pgTable("Users", {
  id: integer("id").primaryKey().generatedAlwaysAsIdentity(),
  name: varchar("name", { length: 255 }).notNull(),
  email: varchar("email", { length: 255 }).notNull().unique(),
  password: varchar("password", { length: 255 }).notNull(),
  role: roleEnum("role").default("driver").notNull(),
  isActive: boolean("isActive").default(true),
  rfidTag: varchar("rfidTag", { length: 255 }).unique(),
  isEmailVerified: boolean("isEmailVerified").default(false).notNull(),
  verificationCode: text("verificationCode"),
  verificationCodeExpiry: timestamp("verificationCodeExpiry"),
  createdAt: timestamp("createdAt").defaultNow().notNull(),
  updatedAt: timestamp("updatedAt").defaultNow().notNull(),
});

// RFIDs table
// Devices table
export const devices = pgTable("Devices", {
  id: uuid("id").primaryKey().defaultRandom(),
  deviceId: varchar("deviceId", { length: 255 }).notNull().unique(),
  macAddress: varchar("macAddress", { length: 255 }).notNull(),
  name: varchar("name", { length: 255 }).notNull(),
  location: varchar("location", { length: 255 }).notNull(),
  apiKey: varchar("apiKey", { length: 255 }).notNull().unique(),
  isActive: boolean("isActive").default(true),
  registrationMode: boolean("registrationMode").default(false),
  pendingRegistrationTagId: varchar("pendingRegistrationTagId", {
    length: 255,
  }).default(""),
  scanMode: boolean("scanMode").default(false),
  lastSeen: timestamp("lastSeen"),
  createdAt: timestamp("createdAt").defaultNow().notNull(),
  updatedAt: timestamp("updatedAt").defaultNow().notNull(),
});

// RFIDs table
export const rfids = pgTable("Rfids", {
  id: uuid("id").primaryKey().defaultRandom(),
  tagId: varchar("tagId", { length: 255 }).notNull().unique(),
  userId: integer("userId").references(() => users.id),
  isActive: boolean("isActive").default(true),
  unitNumber: varchar("unitNumber", { length: 255 }),
  lastScanned: timestamp("lastScanned"),
  deviceId: varchar("deviceId", { length: 255 }).references(
    () => devices.deviceId,
    {
      onDelete: "set null",
      onUpdate: "cascade",
    }
  ),
  registeredBy: integer("registeredBy")
    .notNull()
    .references(() => users.id),
  metadata: json("metadata").default({}),
  createdAt: timestamp("createdAt").defaultNow().notNull(),
  updatedAt: timestamp("updatedAt").defaultNow().notNull(),
});

// RFID Scans table
export const rfidScans = pgTable("RfidScans", {
  id: uuid("id").primaryKey().defaultRandom(),
  rfidTagId: varchar("rfidTagId", { length: 255 }).notNull(),
  deviceId: varchar("deviceId", { length: 255 })
    .notNull()
    .references(() => devices.deviceId, {
      onDelete: "restrict",
      onUpdate: "cascade",
    }),
  userId: integer("userId").references(() => users.id),
  eventType: eventTypeEnum("eventType").default("unknown").notNull(),
  location: varchar("location", { length: 255 }),
  vehicleId: varchar("vehicleId", { length: 255 }),
  scanTime: timestamp("scanTime").defaultNow().notNull(),
  status: scanStatusEnum("status").default("success").notNull(),
  metadata: json("metadata").default({}),
  createdAt: timestamp("createdAt").defaultNow().notNull(),
  updatedAt: timestamp("updatedAt").defaultNow().notNull(),
});

// API Keys table
export const apiKeys = pgTable("ApiKeys", {
  id: uuid("id").primaryKey().defaultRandom(),
  name: varchar("name", { length: 255 }).notNull(),
  deviceId: varchar("deviceId", { length: 255 })
    .notNull()
    .references(() => devices.deviceId, {
      onDelete: "cascade",
      onUpdate: "cascade",
    }),
  description: text("description"),
  key: text("key").notNull().unique(),
  prefix: varchar("prefix", { length: 10 }).notNull(),
  permissions: json("permissions").default(["scan"]).$type<string[]>(),
  lastUsed: timestamp("lastUsed"),
  isActive: boolean("isActive").default(true),
  createdBy: integer("createdBy")
    .notNull()
    .references(() => users.id),
  metadata: json("metadata").default({}),
  type: varchar("type", { length: 50 }).default("device").notNull(),
  createdAt: timestamp("createdAt").defaultNow().notNull(),
  updatedAt: timestamp("updatedAt").defaultNow().notNull(),
});

// Relations
export const usersRelations = relations(users, ({ many, one }) => ({
  registeredRfids: many(rfids, { relationName: "registeredBy" }),
  ownedRfid: one(rfids, {
    fields: [users.rfidTag],
    references: [rfids.tagId],
  }),
  scans: many(rfidScans),
  createdApiKeys: many(apiKeys),
}));

export const rfidsRelations = relations(rfids, ({ one, many }) => ({
  user: one(users, {
    fields: [rfids.userId],
    references: [users.id],
  }),
  registrar: one(users, {
    fields: [rfids.registeredBy],
    references: [users.id],
    relationName: "registeredBy",
  }),
  scans: many(rfidScans),
}));

export const rfidScansRelations = relations(rfidScans, ({ one }) => ({
  rfid: one(rfids, {
    fields: [rfidScans.rfidTagId],
    references: [rfids.tagId],
  }),
  user: one(users, {
    fields: [rfidScans.userId],
    references: [users.id],
  }),
  device: one(devices, {
    fields: [rfidScans.deviceId],
    references: [devices.deviceId],
  }),
}));

export const devicesRelations = relations(devices, ({ many }) => ({
  scans: many(rfidScans),
}));

export const apiKeysRelations = relations(apiKeys, ({ one }) => ({
  creator: one(users, {
    fields: [apiKeys.createdBy],
    references: [users.id],
  }),
}));

// Type exports for TypeScript
export type User = typeof users.$inferSelect;
export type NewUser = typeof users.$inferInsert;

export type Rfid = typeof rfids.$inferSelect;
export type NewRfid = typeof rfids.$inferInsert;

export type RfidScan = typeof rfidScans.$inferSelect;
export type NewRfidScan = typeof rfidScans.$inferInsert;

export type Device = typeof devices.$inferSelect;
export type NewDevice = typeof devices.$inferInsert;

export type ApiKey = typeof apiKeys.$inferSelect;
export type NewApiKey = typeof apiKeys.$inferInsert;
