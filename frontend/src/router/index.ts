import { createRouter, createWebHistory } from "vue-router";
import type { RouteRecordRaw } from "vue-router";
import authService from "../services/auth";

// Import components
const Login = () => import("../views/Login.vue");
const Register = () => import("../views/Register.vue");
const Dashboard = () => import("../views/Dashboard.vue");

const RfidCardManagement = () => import("../views/RfidCardManagement.vue");
const ApiKeyManagement = () => import("../views/ApiKeyManagement.vue");
const DeviceManagement = () => import("../views/DeviceManagement.vue");
const DeviceRegistration = () => import("../views/DeviceRegistration.vue");
const UserManagement = () => import("../views/UserManagement.vue");
const WebSocketTest = () => import("../views/WebSocketTest.vue");
const NotFound = () => import("../views/NotFound.vue");

const routes: RouteRecordRaw[] = [
  {
    path: "/",
    redirect: "/dashboard",
  },
  {
    path: "/login",
    name: "Login",
    component: Login,
    meta: { requiresGuest: true },
  },
  {
    path: "/register",
    name: "Register",
    component: Register,
    meta: { requiresGuest: true },
  },
  {
    path: "/dashboard",
    name: "Dashboard",
    component: Dashboard,
    meta: { requiresAuth: true },
  },
  {
    path: "/rfid",
    name: "RfidManagement",
    component: RfidCardManagement,
    meta: { requiresAuth: true, requiresAdmin: true },
  },
  {
    path: "/users",
    name: "UserManagement",
    component: UserManagement,
    meta: { requiresAuth: true, requiresAdmin: true },
  },
  {
    path: "/apikeys",
    name: "ApiKeyManagement",
    component: ApiKeyManagement,
    meta: { requiresAuth: true, requiresAdmin: true },
  },
  {
    path: "/devices",
    name: "DeviceManagement",
    component: DeviceManagement,
    meta: { requiresAuth: true, requiresAdmin: true },
  },
  {
    path: "/devices/register",
    name: "DeviceRegistration",
    component: DeviceRegistration,
    meta: { requiresAuth: true, requiresAdmin: true },
  },
  {
    path: "/websocket-test",
    name: "WebSocketTest",
    component: WebSocketTest,
    meta: { requiresAuth: true, requiresAdmin: true },
  },
  {
    path: "/:pathMatch(.*)*",
    name: "NotFound",
    component: NotFound,
  },
];

const router = createRouter({
  history: createWebHistory(),
  routes,
});

// Navigation guards
router.beforeEach((to, _, next) => {
  const isLoggedIn = authService.isLoggedIn();
  const isAdmin = authService.isAdmin();

  // Check if route requires guest (not logged in)
  if (to.meta.requiresGuest && isLoggedIn) {
    return next("/dashboard");
  }

  // Check if route requires authentication
  if (to.meta.requiresAuth && !isLoggedIn) {
    return next("/login");
  }

  // Check if route requires admin role
  if (to.meta.requiresAdmin && !isAdmin) {
    return next("/dashboard");
  }

  next();
});

export default router;
