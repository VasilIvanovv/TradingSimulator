# TradingSimulator — Frontend

React + TypeScript single-page application that drives the trading dashboard. Communicates with the C++ backend over HTTP.

---

## Running the frontend

### Prerequisites

- **Node.js ≥ 18** — check with `node --version`
- **npm** — ships with Node (or use any compatible package manager)
- Dependencies already installed (`node_modules/` is present). If you re-clone or delete it, run `npm install` first.

### Start the dev server

```
cd TradingSimulator/FrontEnd
npm run dev
```

Vite starts on **http://localhost:5173** by default and prints the URL. The page hot-reloads on every file save — no manual refresh needed.

> The backend must also be running on **http://localhost:8080** or requests will fail. Start it first via the C++ `main` executable.

### Other commands

| Command | What it does |
|---|---|
| `npm run dev` | Start Vite dev server with HMR |
| `npm run build` | Type-check then compile to `dist/` for production |
| `npm run preview` | Serve the `dist/` build locally (simulates production) |
| `npm run lint` | Run Oxlint static analysis |

---

## Technology stack

| Layer | Choice | Version | Why |
|---|---|---|---|
| UI framework | React | 19 | Industry standard, widely recognized |
| Language | TypeScript | 6 | Static types catch mistakes at compile time |
| Build tool | Vite | 8 | Instant dev server, fast HMR, simple config |
| Routing | React Router | 7 | Declarative nested routing, `<Outlet>` composition |
| Styling | Tailwind CSS | 3 | Utility classes, no separate CSS files, design tokens |
| Linter | Oxlint | 1.7 | Fast Rust-based linter, replaces ESLint |

---

## Project structure

```
FrontEnd/
├── src/
│   ├── main.tsx               # Entry point — mounts <App> into #root
│   ├── App.tsx                # Root component — router + auth provider
│   ├── index.css              # Global styles + Tailwind directives
│   │
│   ├── api/                   # HTTP layer — all fetch() calls live here
│   │   ├── client.ts          # Base fetch wrapper, JWT injection, ApiError
│   │   ├── auth.ts            # register / login calls
│   │   └── trading.ts         # account / orders / rules / history calls
│   │
│   ├── context/
│   │   └── AuthContext.tsx    # Global auth state (JWT + isAuthenticated)
│   │
│   ├── hooks/
│   │   └── useAccount.ts      # Fetch + cache account snapshot
│   │
│   ├── components/
│   │   ├── Layout.tsx         # Sidebar + topbar shell, wraps all protected pages
│   │   └── ProtectedRoute.tsx # Redirects to /login if not authenticated
│   │
│   └── pages/
│       ├── LoginPage.tsx      # /login
│       ├── RegisterPage.tsx   # /register
│       └── DashboardPage.tsx  # / (protected)
│
├── index.html                 # HTML shell — contains <div id="root">
├── vite.config.ts             # Vite config (plugin-react only)
├── tailwind.config.js         # Design tokens (colors, fonts)
├── tsconfig.json              # Root TS config (references app + node)
├── tsconfig.app.json          # App compiler options
└── package.json               # Dependencies and scripts
```

---

## Architecture

### How a request flows end-to-end

```
User action (click / form submit)
    ↓
Page component (e.g. LoginPage)
    ↓
Context method or direct API call (e.g. useAuth().login())
    ↓
api/auth.ts or api/trading.ts  — typed wrapper
    ↓
api/client.ts — request()  — attaches JWT, calls fetch()
    ↓
C++ backend  http://localhost:8080
    ↓
Response JSON parsed → typed return value
    ↑  (error path)
ApiError thrown if !res.ok
    ↑
Caught in the page component → shown in UI
```

### Route structure

```
/login              LoginPage        (public)
/register           RegisterPage     (public)
/                   DashboardPage    (protected — requires JWT)
/trade              TradePage        (protected — placeholder)
/rules              RulesPage        (protected — placeholder)
/history            HistoryPage      (protected — placeholder)
*                   → redirect to /
```

Protected routes are wrapped in two nested route elements:

1. **`<ProtectedRoute>`** — checks `isAuthenticated`. If false, redirects to `/login` before rendering anything.
2. **`<Layout>`** — renders the sidebar + topbar shell. Uses `<Outlet>` to inject the matched child page into the content area.

The nesting in `App.tsx` looks like:

```tsx
<Route element={<ProtectedRoute />}>
  <Route element={<Layout />}>
    <Route path="/" element={<DashboardPage />} />
    ...
  </Route>
</Route>
```

React Router resolves this by calling each element's `<Outlet>` in turn: `ProtectedRoute` renders `Layout`, `Layout` renders the matched page.

---

## Authentication

### How the JWT is stored

The JWT token lives in **two places in memory only** — never in `localStorage` or cookies:

1. A module-level variable `let token` in `api/client.ts`
2. A boolean `isAuthenticated` in React state inside `AuthContext`

The module variable is what actually gets sent in requests (`Authorization: Bearer <token>`). The React state is what components read to decide what to render. They are set together in `AuthContext.login()` / `AuthContext.register()`.

**Why not `localStorage`?** Any JavaScript on the page can read `localStorage`, including injected scripts from XSS attacks. A module-level variable is inaccessible from outside the module. The tradeoff is that the token is gone on page refresh, requiring the user to log in again.

### Login flow step by step

1. User fills in the form and submits.
2. `LoginPage.handleSubmit` calls `useAuth().login(username, password)`.
3. `AuthContext.login` calls `authApi.login(username, password)`.
4. `authApi.login` calls `api.post('/auth/login', { username, password })`.
5. `client.request` sends `POST http://localhost:8080/auth/login` with JSON body.
6. Backend validates credentials (Argon2 hash verify via libsodium), returns `{ "token": "<jwt>" }`.
7. `AuthContext.login` receives the response, calls `setToken(token)` (sets the module var) and `setIsAuthenticated(true)`.
8. `LoginPage` gets no exception thrown, calls `navigate('/', { replace: true })`.
9. React Router re-evaluates routes — `isAuthenticated` is now true, `ProtectedRoute` renders `Layout` with `DashboardPage`.

### Error flow (wrong password)

Steps 1–5 same. At step 6, the backend returns HTTP 401 with `{ "error": "Invalid username or password" }`. In `client.request`, `!res.ok` is true, so it throws `new ApiError(401, "Invalid username or password")`. `LoginPage` catches this in the `catch` block and sets `error` state, which renders the red error banner.

---

## API layer (`src/api/`)

### `client.ts` — the base

```
BASE = 'http://localhost:8080'
```

`request<T>(path, init)` is the single function all API calls go through:

- Builds the `headers` object with `Content-Type: application/json` and optionally `Authorization: Bearer <token>` if `token` is set.
- Calls `fetch(BASE + path, { ...init, headers })`.
- If `!res.ok`: reads the response body as JSON, extracts `body.error` (the backend always puts the error string in `{ "error": "..." }`), and throws `ApiError(status, message)`.
- If ok: returns `res.json()` cast to `T`.

`ApiError` extends `Error` and adds a `status: number` field. Pages use `instanceof ApiError` to distinguish server-returned errors from network failures.

### `auth.ts`

Two calls: `register` and `login`. Both POST to `/auth/register` and `/auth/login` respectively and return `AuthResponse { token: string }`.

### `trading.ts`

Typed interfaces that mirror the backend JSON exactly:

| Interface | Backend source | Description |
|---|---|---|
| `AccountSnapshot` | `GET /account` | Cash balance, positions map, trade history |
| `TradeRecord` | inside `AccountSnapshot.tradeHistory` | One completed trade |
| `ExecutionReceipt` | `POST /orders` response | Result of placing an order |
| `PriceCandle` | `GET /history` response | OHLCV bar |

`tradingApi` object exposes:

| Method | HTTP call | Purpose |
|---|---|---|
| `getAccount()` | `GET /account` | Fetch full account state |
| `placeOrder(symbol, side, qty, price)` | `POST /orders` | Place a buy or sell |
| `addRule(symbol, triggerPrice, side, qty)` | `POST /rules` | Add a price-trigger rule |
| `removeRules(symbol)` | `DELETE /rules/:symbol` | Remove all rules for a symbol |
| `getHistory(symbol, interval, start)` | `GET /history?...` | Fetch OHLCV candles |

---

## State management

There is no external state library (no Redux, no Zustand). State is kept at the lowest level that needs it:

| State | Where it lives | Why there |
|---|---|---|
| JWT token | Module var in `client.ts` | Injected into every request; not a display concern |
| `isAuthenticated` | `AuthContext` (React state) | Needed by `ProtectedRoute` and `Layout` — must be React state so re-renders trigger |
| Account snapshot | `useAccount` hook (local state) | Only needed by the components that call the hook |
| Form fields, loading, error | Each page's local `useState` | Not shared; scoped to one form |

### `useAccount` hook

`useAccount()` wraps `tradingApi.getAccount()` with a standard async loading pattern:

```
state: { account, loading, error }

mount / refresh()
  → setLoading(true), setError(null)
  → await tradingApi.getAccount()
  → success: setAccount(data), setLoading(false)
  → failure: setError(message), setLoading(false)
```

`refresh` is a `useCallback` with no dependencies (stable reference). The `useEffect` depends on `[fetch]` — this means it runs exactly once on mount. Calling `refresh()` manually (e.g. after placing a trade) re-fetches without remounting.

`useAccount` is called in **two places**: `Layout` (to show cash/positions/trades in the topbar) and `DashboardPage` (to render the cards). These are two independent hook instances — two separate fetches. They are intentionally not shared because the Layout fetch is for display only and the Dashboard fetch drives the full detail view. If this becomes a problem (double requests), the fix is to lift `useAccount` into a context.

---

## Layout and styling

### Design tokens

All colors are defined once in `tailwind.config.js` and referenced everywhere as Tailwind utility classes:

| Token | Hex | Usage |
|---|---|---|
| `bg` | `#070D1A` | Page background |
| `surface` | `#0C1526` | Sidebar background |
| `card` | `#101C34` | Card backgrounds |
| `border` | `#1A2D4A` | Standard borders |
| `border-dim` | `#0F1D31` | Subtle dividers (table rows, section breaks) |
| `txt` | `#C9D8EC` | Primary text |
| `txt-2` | `#5D7898` | Secondary / dimmed text |
| `txt-3` | `#3A5270` | Placeholder / label text |
| `accent` | `#00CFAB` | Teal — primary interactive color, active nav |
| `pos` | `#00CFAB` | Positive values (same as accent) |
| `neg` | `#FF4158` | Negative values, errors |
| `link` | `#4D9FFF` | Hyperlinks |

Using tokens instead of hex values directly means changing a color is a one-line edit in `tailwind.config.js` with no search-replace across files.

### `index.css`

The only global CSS file. Sets:
- Box-sizing reset (`*, *::before, *::after`)
- Body defaults (background, color, font, antialiasing)
- `#root { height: 100vh }` — required so the flex layout fills the viewport
- `font-variant-numeric: tabular-nums` on `.font-mono` — numbers align in columns
- Thin custom scrollbar (4px, matches the dark theme)

### `Layout.tsx` structure

```
<div class="flex h-screen">               ← full viewport, no scroll
  <aside class="w-[196px] flex-col">      ← sidebar, fixed width
    Brand logo
    <nav>                                 ← grouped by section (Overview / Trading / Data)
      <NavItem> per route
    </nav>
    User row + Sign out button
  </aside>

  <div class="flex-1 flex-col min-w-0">  ← right side, takes remaining space
    <div class="h-[52px]">               ← topbar, fixed height
      Cash / Positions / Trades stats
    </div>
    <main class="flex-1 overflow-y-auto"> ← scrollable page content
      <Outlet />                          ← matched page renders here
    </main>
  </div>
</div>
```

`min-w-0` on the right column prevents flex overflow when content is wide — without it, long content forces the column wider than its flex share.

`NavItem` uses React Router's `<NavLink>` with a callback `className` that receives `{ isActive }`. The active state applies a left border stripe (`border-l-2 border-accent`) and a faint teal background tint (`bg-accent/10`). The `end` prop on the Dashboard entry means it only matches exactly `/`, not `/trade`, `/rules`, etc.

---

## TypeScript configuration

Two tsconfig files:

- **`tsconfig.app.json`** — compiles `src/`. Key flags:
  - `"noEmit": true` — TypeScript only type-checks; Vite handles the actual transpile via its own esbuild pipeline
  - `"moduleResolution": "bundler"` — allows importing `.tsx` extensions directly (Vite-style)
  - `"verbatimModuleSyntax": true` — enforces `import type` for type-only imports, prevents runtime import of types
  - `"noUnusedLocals"` + `"noUnusedParameters"` — catch dead code
  - `"jsx": "react-jsx"` — uses the React 17+ automatic JSX transform (no need to `import React` in every file)

- **`tsconfig.node.json`** — compiles `vite.config.ts` under Node.js context. Separate because the browser and Node environments have different globals.

---

## How to add a new page

1. Create `src/pages/MyPage.tsx` and export a named component.
2. Add the route in `App.tsx` inside the `<Route element={<Layout />}>` block:
   ```tsx
   <Route path="/my-page" element={<MyPage />} />
   ```
3. Add a `NavItem` entry to the `NAV` array in `Layout.tsx`:
   ```ts
   { label: 'My Page', to: '/my-page', exact: false, icon: <svg ...> }
   ```
4. Move it into the correct nav section by adjusting the `NAV.slice()` ranges.

No registration, no barrel files, no additional config needed.

## How to add a new API call

1. Add the TypeScript interface for the response shape to `src/api/trading.ts`.
2. Add the method to the `tradingApi` object:
   ```ts
   myCall: (param: string) => api.get<MyResponse>(`/endpoint/${param}`)
   ```
3. Call `tradingApi.myCall(...)` from your component or hook.

The JWT is injected automatically by `client.request` — you never touch `Authorization` headers in individual calls.
