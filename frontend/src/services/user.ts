import apiClient from "./api";
import type { ApiResponse } from "./api";

// User service interfaces
export interface UserCredentials {
  name?: string;
  email: string;
  password: string;
  role?: string;
  isActive?: boolean;
}

export interface UserUpdateData {
  name?: string;
  email?: string;
  password?: string;
  role?: string;
  isActive?: boolean;
}

export interface User {
  id: number;
  name: string;
  email: string;
  role: "driver" | "admin" | "superadmin";
  isActive: boolean;
  rfidTag?: string;
  createdAt: string;
  updatedAt: string;
}

/**
 * Client-side email validation (matches backend RFC 5321)
 */
const validateEmail = (email: string): { valid: boolean; error?: string } => {
  if (!email || email.length === 0) {
    return { valid: false, error: "Email is required" };
  }

  if (email.length > 320) {
    return { valid: false, error: "Email is too long" };
  }

  const emailRegex =
    /^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/;

  if (!emailRegex.test(email)) {
    return { valid: false, error: "Invalid email format" };
  }

  return { valid: true };
};

/**
 * Client-side name validation
 */
const validateName = (name: string): { valid: boolean; error?: string } => {
  if (!name || name.trim().length === 0) {
    return { valid: false, error: "Name is required" };
  }

  if (name.length < 2 || name.length > 100) {
    return { valid: false, error: "Name must be between 2 and 100 characters" };
  }

  return { valid: true };
};

// Get all users
const getUsers = async (): Promise<ApiResponse<User[]>> => {
  try {
    const response = await apiClient.get("/users");
    return response.data || { success: false, data: [] };
  } catch (error: any) {
    console.error("Failed to fetch users:", error);
    throw new Error(error.message || "Failed to fetch users");
  }
};

// Get a specific user
const getUser = async (id: number): Promise<User> => {
  try {
    const response = await apiClient.get(`/users/${id}`);
    return response.data;
  } catch (error: any) {
    console.error(`Failed to fetch user ${id}:`, error);
    throw new Error(error.message || "Failed to fetch user");
  }
};

// Create a new user
const createUser = async (userData: UserCredentials): Promise<User> => {
  // Client-side validation
  if (userData.email) {
    const emailValidation = validateEmail(userData.email);
    if (!emailValidation.valid) {
      throw new Error(emailValidation.error);
    }
  }

  if (userData.name) {
    const nameValidation = validateName(userData.name);
    if (!nameValidation.valid) {
      throw new Error(nameValidation.error);
    }
  }

  try {
    const response = await apiClient.post("/users", userData);
    return response.data;
  } catch (error: any) {
    // Handle validation errors from backend
    if (error.response?.status === 400) {
      const apiResponse = error.response.data as ApiResponse;
      if (apiResponse.errors && apiResponse.errors.length > 0) {
        throw new Error(apiResponse.errors.join(", "));
      }
    }
    throw new Error(error.message || "Failed to create user");
  }
};

// Update a user
const updateUser = async (
  id: number,
  userData: UserUpdateData
): Promise<User> => {
  // Client-side validation
  if (userData.email) {
    const emailValidation = validateEmail(userData.email);
    if (!emailValidation.valid) {
      throw new Error(emailValidation.error);
    }
  }

  if (userData.name) {
    const nameValidation = validateName(userData.name);
    if (!nameValidation.valid) {
      throw new Error(nameValidation.error);
    }
  }

  try {
    const response = await apiClient.put(`/users/${id}`, userData);
    return response.data;
  } catch (error: any) {
    if (error.response?.status === 400) {
      const apiResponse = error.response.data as ApiResponse;
      if (apiResponse.errors && apiResponse.errors.length > 0) {
        throw new Error(apiResponse.errors.join(", "));
      }
    }
    throw new Error(error.message || "Failed to update user");
  }
};

// Delete a user
const deleteUser = async (id: number): Promise<void> => {
  try {
    await apiClient.delete(`/users/${id}`);
  } catch (error: any) {
    console.error(`Failed to delete user ${id}:`, error);
    throw new Error(error.message || "Failed to delete user");
  }
};

export default {
  getUsers,
  getUser,
  createUser,
  updateUser,
  deleteUser,
  validateEmail,
  validateName,
};
