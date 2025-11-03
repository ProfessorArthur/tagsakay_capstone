# TagSakay Frontend

Frontend application for the TagSakay RFID-based queue management system, built with Vue 3, TypeScript, and Tailwind CSS.

## Overview

The TagSakay frontend provides a user-friendly interface to manage RFID tags, API keys, and view system status. It's designed to work seamlessly with the TagSakay backend API.

## Features

- **Authentication**: Secure login and registration with JWT token-based authentication
- **Dashboard**: System overview with key metrics and status indicators
- **RFID Management**: Register, activate, deactivate, and monitor RFID tags
- **API Key Management**: Create and manage API keys for device authentication
- **Role-Based Access Control**: Different views and permissions based on user roles
- **Responsive Design**: Mobile-friendly interface using Tailwind CSS and DaisyUI

## Tech Stack

- **Vue 3**: Progressive JavaScript framework
- **TypeScript**: Type safety and better developer experience
- **Vite**: Next-generation frontend tooling
- **Vue Router**: Official router for Vue.js
- **Axios**: Promise-based HTTP client
- **Tailwind CSS**: Utility-first CSS framework
- **DaisyUI**: Component library for Tailwind CSS

## Getting Started

### Prerequisites

- Node.js (v16 or newer)
- npm or yarn

### Installation

1. Clone the repository
2. Navigate to the frontend directory:
   ```bash
   cd frontend
   ```
3. Install dependencies:
   ```bash
   npm install
   # or
   yarn
   ```

### Development

Start the development server:

```bash
npm run dev
# or
yarn dev
```

The application will be available at `http://localhost:5173` by default.

### Building for Production

```bash
npm run build
# or
yarn build
```

The built files will be in the `dist` directory.

## Environment Variables

Create a `.env` file in the root of the frontend directory with the following variables:

```
VITE_API_URL=http://localhost:3000/api
```

## Authentication

The application uses JWT tokens for authentication. On successful login, the token is stored in localStorage and automatically included in API requests.

## User Roles

- **Admin**: Full access to all features
- **Driver**: Limited access to dashboard and personal information

## Test Accounts

The following test accounts are available for development:

- Admin: admin@tagsakay.com / Admin@123
- Dispatcher: dispatcher@tagsakay.com / Dispatch@123
- Driver 1: driver1@tagsakay.com / Driver@123
- Driver 2: driver2@tagsakay.com / Driver@123
- Passenger: passenger1@example.com / Pass@123
