import { createContext, useContext, useState, ReactNode } from 'react'
import { authApi } from '../api/auth'
import { setToken, getToken } from '../api/client'

interface AuthContextValue {
  isAuthenticated: boolean
  login: (username: string, password: string) => Promise<void>
  register: (username: string, password: string) => Promise<void>
  logout: () => void
}

const AuthContext = createContext<AuthContextValue | null>(null)

export function AuthProvider({ children }: { children: ReactNode }) {
  const [isAuthenticated, setIsAuthenticated] = useState(() => getToken() !== null)

  async function login(username: string, password: string) {
    const { token } = await authApi.login(username, password)
    setToken(token)
    setIsAuthenticated(true)
  }

  async function register(username: string, password: string) {
    const { token } = await authApi.register(username, password)
    setToken(token)
    setIsAuthenticated(true)
  }

  function logout() {
    setToken(null)
    setIsAuthenticated(false)
  }

  return (
    <AuthContext.Provider value={{ isAuthenticated, login, register, logout }}>
      {children}
    </AuthContext.Provider>
  )
}

export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used inside AuthProvider')
  return ctx
}
