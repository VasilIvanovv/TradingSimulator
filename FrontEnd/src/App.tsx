import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { AuthProvider } from './context/AuthContext'
import { ActiveAccountProvider } from './context/ActiveAccountContext'
import { ProtectedRoute } from './components/ProtectedRoute'
import { Layout } from './components/Layout'
import { LoginPage } from './pages/LoginPage'
import { RegisterPage } from './pages/RegisterPage'
import { DashboardPage } from './pages/DashboardPage'
import { RulesPage } from './pages/RulesPage'
import { MarketsPage } from './pages/MarketsPage'
import { StockPage } from './pages/StockPage'

export default function App() {
  return (
    <AuthProvider>
      <ActiveAccountProvider>
      <BrowserRouter>
        <Routes>
          <Route path="/login"    element={<LoginPage />} />
          <Route path="/register" element={<RegisterPage />} />

          <Route element={<ProtectedRoute />}>
            <Route element={<Layout />}>
              <Route path="/"        element={<DashboardPage />} />
              <Route path="/markets"          element={<MarketsPage />} />
              <Route path="/stocks/:symbol"  element={<StockPage />} />
              <Route path="/rules"           element={<RulesPage />} />
              <Route path="/history" element={<Placeholder title="History" />} />
            </Route>
          </Route>

          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </BrowserRouter>
      </ActiveAccountProvider>
    </AuthProvider>
  )
}

function Placeholder({ title }: { title: string }) {
  return (
    <div className="flex items-center justify-center h-48 text-txt-3 text-sm">
      {title} — coming soon
    </div>
  )
}
