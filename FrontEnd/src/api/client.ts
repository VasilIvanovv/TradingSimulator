export const BASE = 'http://localhost:8080'

const TOKEN_KEY     = 'ts_token'
const ACCOUNT_KEY   = 'ts_account_id'

let token:     string | null = sessionStorage.getItem(TOKEN_KEY)
let accountId: number | null = (() => {
  const v = sessionStorage.getItem(ACCOUNT_KEY)
  return v !== null ? parseInt(v, 10) : null
})()

export function setToken(t: string | null) {
  token = t
  if (t) sessionStorage.setItem(TOKEN_KEY, t)
  else   sessionStorage.removeItem(TOKEN_KEY)
}
export function getToken() { return token }

export function setAccountId(id: number | null) {
  accountId = id
  if (id !== null) sessionStorage.setItem(ACCOUNT_KEY, String(id))
  else             sessionStorage.removeItem(ACCOUNT_KEY)
}
export function getAccountId() { return accountId }

// Appends ?accountId=X to account-scoped paths.
export function withAccount(path: string): string {
  if (accountId === null) return path
  const sep = path.includes('?') ? '&' : '?'
  return `${path}${sep}accountId=${accountId}`
}

async function request<T>(path: string, init: RequestInit = {}): Promise<T> {
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(init.headers as Record<string, string>),
  }
  if (token) headers['Authorization'] = `Bearer ${token}`

  let res: Response
  try {
    res = await fetch(`${BASE}${path}`, { ...init, headers })
  } catch {
    throw new ApiError(0, 'Unable to connect. Please try again later.')
  }

  if (!res.ok) {
    const body = await res.json().catch(() => ({}))
    throw new ApiError(res.status, body.error ?? res.statusText)
  }

  if (res.status === 204) return undefined as T
  return res.json() as Promise<T>
}

export class ApiError extends Error {
  constructor(public status: number, message: string) {
    super(message)
  }
}

export const api = {
  post: <T>(path: string, body: unknown) =>
    request<T>(path, { method: 'POST', body: JSON.stringify(body) }),

  put: <T>(path: string, body: unknown) =>
    request<T>(path, { method: 'PUT', body: JSON.stringify(body) }),

  get: <T>(path: string) =>
    request<T>(path, { method: 'GET' }),

  delete: <T>(path: string) =>
    request<T>(path, { method: 'DELETE' }),
}
