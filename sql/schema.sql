-- =============================================================================
-- EDV System - Enterprise Resource Planning Database Schema
-- PostgreSQL 14+
-- Run this script as superuser to initialize the database.
-- =============================================================================

-- Create database (run separately as superuser if needed):
-- CREATE DATABASE edv_system ENCODING 'UTF8';

-- =============================================================================
-- A. CORE SYSTEM & SECURITY
-- =============================================================================

CREATE TABLE IF NOT EXISTS roles (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(50) UNIQUE NOT NULL,
    description TEXT,
    permissions JSONB DEFAULT '{}'::jsonb
);

CREATE TABLE IF NOT EXISTS users (
    id            SERIAL PRIMARY KEY,
    username      VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(256) NOT NULL,    -- SHA-256 hex
    email         VARCHAR(255),
    role          VARCHAR(50) NOT NULL DEFAULT 'user',
    is_active     BOOLEAN NOT NULL DEFAULT true,
    last_login    TIMESTAMPTZ,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS company_codes (
    id          SERIAL PRIMARY KEY,
    code        VARCHAR(10) UNIQUE NOT NULL,
    name        VARCHAR(255) NOT NULL,
    address     TEXT,
    tax_id      VARCHAR(50),
    currency    VARCHAR(10) DEFAULT 'EUR',
    is_active   BOOLEAN NOT NULL DEFAULT true
);

-- =============================================================================
-- B. MASTER DATA
-- =============================================================================

CREATE TABLE IF NOT EXISTS customers (
    id               SERIAL PRIMARY KEY,
    customer_number  VARCHAR(20) UNIQUE NOT NULL,
    name             VARCHAR(255) NOT NULL,
    tax_id           VARCHAR(50),
    email            VARCHAR(255),
    phone            VARCHAR(50),
    billing_address  TEXT,
    shipping_address TEXT,
    payment_terms    INTEGER DEFAULT 30,   -- days
    credit_limit     NUMERIC(15,2) DEFAULT 0,
    currency         VARCHAR(10) DEFAULT 'EUR',
    notes            TEXT,
    is_active        BOOLEAN NOT NULL DEFAULT true,
    created_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at       TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS vendors (
    id              SERIAL PRIMARY KEY,
    vendor_number   VARCHAR(20) UNIQUE NOT NULL,
    name            VARCHAR(255) NOT NULL,
    tax_id          VARCHAR(50),
    email           VARCHAR(255),
    phone           VARCHAR(50),
    address         TEXT,
    bank_name       VARCHAR(255),
    bank_iban       VARCHAR(50),
    bank_bic        VARCHAR(20),
    payment_terms   INTEGER DEFAULT 30,
    currency        VARCHAR(10) DEFAULT 'EUR',
    notes           TEXT,
    is_active       BOOLEAN NOT NULL DEFAULT true,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS material_categories (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(100) NOT NULL,
    parent_id   INTEGER REFERENCES material_categories(id),
    description TEXT
);

CREATE TABLE IF NOT EXISTS materials (
    id            SERIAL PRIMARY KEY,
    sku           VARCHAR(50) UNIQUE NOT NULL,
    name          VARCHAR(255) NOT NULL,
    description   TEXT,
    category_id   INTEGER REFERENCES material_categories(id),
    base_unit     VARCHAR(20) DEFAULT 'pcs',  -- pcs, kg, ltr, m
    cost_price    NUMERIC(15,4) DEFAULT 0,
    selling_price NUMERIC(15,4) DEFAULT 0,
    min_stock     NUMERIC(15,3) DEFAULT 0,
    max_stock     NUMERIC(15,3) DEFAULT 0,
    barcode       VARCHAR(100),
    weight_kg     NUMERIC(10,3),
    is_active     BOOLEAN NOT NULL DEFAULT true,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS warehouses (
    id          SERIAL PRIMARY KEY,
    code        VARCHAR(20) UNIQUE NOT NULL,
    name        VARCHAR(255) NOT NULL,
    address     TEXT,
    is_active   BOOLEAN NOT NULL DEFAULT true
);

CREATE TABLE IF NOT EXISTS departments (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(100) NOT NULL,
    manager_id  INTEGER,   -- references employees.id (set after table creation)
    is_active   BOOLEAN NOT NULL DEFAULT true
);

CREATE TABLE IF NOT EXISTS employees (
    id              SERIAL PRIMARY KEY,
    employee_number VARCHAR(20) UNIQUE NOT NULL,
    first_name      VARCHAR(100) NOT NULL,
    last_name       VARCHAR(100) NOT NULL,
    email           VARCHAR(255),
    phone           VARCHAR(50),
    department_id   INTEGER REFERENCES departments(id),
    job_title       VARCHAR(100),
    hire_date       DATE,
    birth_date      DATE,
    salary          NUMERIC(12,2),
    currency        VARCHAR(10) DEFAULT 'EUR',
    manager_id      INTEGER REFERENCES employees(id),
    address         TEXT,
    iban            VARCHAR(50),
    is_active       BOOLEAN NOT NULL DEFAULT true,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Now add FK for departments.manager_id
ALTER TABLE departments
    ADD CONSTRAINT fk_dept_manager
    FOREIGN KEY (manager_id) REFERENCES employees(id);

-- =============================================================================
-- C. TRANSACTIONAL DATA
-- =============================================================================

CREATE TABLE IF NOT EXISTS sales_orders (
    id              SERIAL PRIMARY KEY,
    order_number    VARCHAR(30) UNIQUE NOT NULL,
    customer_id     INTEGER NOT NULL REFERENCES customers(id),
    order_date      DATE NOT NULL DEFAULT CURRENT_DATE,
    delivery_date   DATE,
    status          VARCHAR(30) NOT NULL DEFAULT 'draft',  -- draft/confirmed/shipped/invoiced/paid/cancelled
    currency        VARCHAR(10) DEFAULT 'EUR',
    total_net       NUMERIC(15,2) DEFAULT 0,
    total_tax       NUMERIC(15,2) DEFAULT 0,
    total_gross     NUMERIC(15,2) DEFAULT 0,
    notes           TEXT,
    created_by      INTEGER REFERENCES users(id),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS sales_order_lines (
    id              SERIAL PRIMARY KEY,
    order_id        INTEGER NOT NULL REFERENCES sales_orders(id) ON DELETE CASCADE,
    line_number     INTEGER NOT NULL,
    material_id     INTEGER NOT NULL REFERENCES materials(id),
    description     TEXT,
    quantity        NUMERIC(15,3) NOT NULL,
    unit_price      NUMERIC(15,4) NOT NULL,
    discount_pct    NUMERIC(5,2) DEFAULT 0,
    tax_rate        NUMERIC(5,2) DEFAULT 19,   -- % VAT
    net_amount      NUMERIC(15,2) NOT NULL,
    tax_amount      NUMERIC(15,2) NOT NULL,
    gross_amount    NUMERIC(15,2) NOT NULL
);

CREATE TABLE IF NOT EXISTS purchase_orders (
    id              SERIAL PRIMARY KEY,
    po_number       VARCHAR(30) UNIQUE NOT NULL,
    vendor_id       INTEGER NOT NULL REFERENCES vendors(id),
    order_date      DATE NOT NULL DEFAULT CURRENT_DATE,
    expected_date   DATE,
    status          VARCHAR(30) NOT NULL DEFAULT 'draft',
    currency        VARCHAR(10) DEFAULT 'EUR',
    total_net       NUMERIC(15,2) DEFAULT 0,
    total_tax       NUMERIC(15,2) DEFAULT 0,
    total_gross     NUMERIC(15,2) DEFAULT 0,
    notes           TEXT,
    created_by      INTEGER REFERENCES users(id),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS purchase_order_lines (
    id              SERIAL PRIMARY KEY,
    po_id           INTEGER NOT NULL REFERENCES purchase_orders(id) ON DELETE CASCADE,
    line_number     INTEGER NOT NULL,
    material_id     INTEGER NOT NULL REFERENCES materials(id),
    quantity        NUMERIC(15,3) NOT NULL,
    unit_price      NUMERIC(15,4) NOT NULL,
    net_amount      NUMERIC(15,2) NOT NULL
);

CREATE TABLE IF NOT EXISTS inventory_ledger (
    id              SERIAL PRIMARY KEY,
    material_id     INTEGER NOT NULL REFERENCES materials(id),
    warehouse_id    INTEGER NOT NULL REFERENCES warehouses(id),
    quantity_change NUMERIC(15,3) NOT NULL,  -- positive=in, negative=out
    unit_cost       NUMERIC(15,4),
    movement_type   VARCHAR(30) NOT NULL,    -- purchase/sale/adjustment/transfer
    reference_id    INTEGER,                 -- sales_order or purchase_order id
    reference_type  VARCHAR(30),             -- 'sales_order' / 'purchase_order'
    notes           TEXT,
    created_by      INTEGER REFERENCES users(id),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- View: current stock per material/warehouse
CREATE OR REPLACE VIEW stock_levels AS
SELECT
    m.id            AS material_id,
    m.sku,
    m.name          AS material_name,
    w.id            AS warehouse_id,
    w.name          AS warehouse_name,
    SUM(il.quantity_change) AS quantity_on_hand,
    m.min_stock,
    m.selling_price
FROM inventory_ledger il
JOIN materials m ON m.id = il.material_id
JOIN warehouses w ON w.id = il.warehouse_id
GROUP BY m.id, m.sku, m.name, w.id, w.name, m.min_stock, m.selling_price;

-- =============================================================================
-- D. FINANCIALS / ACCOUNTING (FICO)
-- =============================================================================

CREATE TABLE IF NOT EXISTS chart_of_accounts (
    id              SERIAL PRIMARY KEY,
    account_number  VARCHAR(20) UNIQUE NOT NULL,
    name            VARCHAR(255) NOT NULL,
    account_type    VARCHAR(30) NOT NULL,   -- asset/liability/equity/revenue/expense
    parent_id       INTEGER REFERENCES chart_of_accounts(id),
    is_active       BOOLEAN NOT NULL DEFAULT true
);

CREATE TABLE IF NOT EXISTS journal_entries (
    id              SERIAL PRIMARY KEY,
    entry_number    VARCHAR(30) UNIQUE NOT NULL,
    entry_date      DATE NOT NULL DEFAULT CURRENT_DATE,
    description     TEXT NOT NULL,
    reference_type  VARCHAR(30),   -- 'sales_order' / 'purchase_order' / 'manual'
    reference_id    INTEGER,
    is_posted       BOOLEAN NOT NULL DEFAULT false,
    created_by      INTEGER REFERENCES users(id),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS journal_entry_lines (
    id              SERIAL PRIMARY KEY,
    entry_id        INTEGER NOT NULL REFERENCES journal_entries(id) ON DELETE CASCADE,
    account_id      INTEGER NOT NULL REFERENCES chart_of_accounts(id),
    debit_amount    NUMERIC(15,2) DEFAULT 0,
    credit_amount   NUMERIC(15,2) DEFAULT 0,
    description     TEXT
);

-- =============================================================================
-- E. HR - LEAVE MANAGEMENT
-- =============================================================================

CREATE TABLE IF NOT EXISTS leave_types (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(100) NOT NULL,
    days_per_year INTEGER DEFAULT 0,
    is_paid     BOOLEAN DEFAULT true
);

CREATE TABLE IF NOT EXISTS leave_requests (
    id              SERIAL PRIMARY KEY,
    employee_id     INTEGER NOT NULL REFERENCES employees(id),
    leave_type_id   INTEGER NOT NULL REFERENCES leave_types(id),
    start_date      DATE NOT NULL,
    end_date        DATE NOT NULL,
    days_count      INTEGER NOT NULL,
    reason          TEXT,
    status          VARCHAR(20) DEFAULT 'pending',   -- pending/approved/rejected
    approved_by     INTEGER REFERENCES employees(id),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- F. AUDIT LOG
-- =============================================================================

CREATE TABLE IF NOT EXISTS audit_log (
    id          BIGSERIAL PRIMARY KEY,
    table_name  VARCHAR(100) NOT NULL,
    record_id   INTEGER,
    action      VARCHAR(10) NOT NULL,  -- INSERT / UPDATE / DELETE
    changed_by  INTEGER REFERENCES users(id),
    changed_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    old_data    JSONB,
    new_data    JSONB
);

-- =============================================================================
-- G. SEQUENCES FOR DOCUMENT NUMBERS
-- =============================================================================

CREATE SEQUENCE IF NOT EXISTS seq_sales_order START 1000 INCREMENT 1;
CREATE SEQUENCE IF NOT EXISTS seq_purchase_order START 1000 INCREMENT 1;
CREATE SEQUENCE IF NOT EXISTS seq_journal_entry START 1000 INCREMENT 1;
CREATE SEQUENCE IF NOT EXISTS seq_customer START 10000 INCREMENT 1;
CREATE SEQUENCE IF NOT EXISTS seq_vendor START 10000 INCREMENT 1;
CREATE SEQUENCE IF NOT EXISTS seq_employee START 1000 INCREMENT 1;

-- =============================================================================
-- H. INDEXES
-- =============================================================================

CREATE INDEX IF NOT EXISTS idx_customers_name ON customers(name);
CREATE INDEX IF NOT EXISTS idx_materials_sku ON materials(sku);
CREATE INDEX IF NOT EXISTS idx_sales_orders_customer ON sales_orders(customer_id);
CREATE INDEX IF NOT EXISTS idx_sales_orders_date ON sales_orders(order_date);
CREATE INDEX IF NOT EXISTS idx_inventory_material ON inventory_ledger(material_id);
CREATE INDEX IF NOT EXISTS idx_journal_date ON journal_entries(entry_date);
CREATE INDEX IF NOT EXISTS idx_employees_dept ON employees(department_id);

-- =============================================================================
-- I. SEED DATA
-- =============================================================================

-- Default roles
INSERT INTO roles (name, description) VALUES
    ('admin',     'Full system access'),
    ('manager',   'Management access to all modules'),
    ('sales',     'CRM and Sales access'),
    ('warehouse', 'Inventory access'),
    ('hr',        'Human Resources access'),
    ('finance',   'Finance and Accounting access'),
    ('user',      'Basic read-only access')
ON CONFLICT (name) DO NOTHING;

-- Default company
INSERT INTO company_codes (code, name, address, currency) VALUES
    ('MAIN', 'My Company GmbH', 'Musterstrasse 1, 12345 Berlin, Germany', 'EUR')
ON CONFLICT (code) DO NOTHING;

-- Default warehouse
INSERT INTO warehouses (code, name) VALUES
    ('WH01', 'Main Warehouse')
ON CONFLICT (code) DO NOTHING;

-- Default admin user (password = "Admin1234!" - CHANGE AFTER FIRST LOGIN)
-- SHA-256 hex of "Admin1234!" = must be computed by the application
-- Use the app's "Create Admin" flow on first run, or run:
-- UPDATE users SET password_hash = encode(sha256('Admin1234!'::bytea), 'hex') WHERE username = 'admin';
-- (requires pgcrypto extension)

-- Default leave types
INSERT INTO leave_types (name, days_per_year, is_paid) VALUES
    ('Annual Leave',   30, true),
    ('Sick Leave',     30, true),
    ('Unpaid Leave',    0, false),
    ('Maternity Leave', 180, true),
    ('Paternity Leave', 14, true)
ON CONFLICT DO NOTHING;

-- Default chart of accounts (German SKR03 simplified)
INSERT INTO chart_of_accounts (account_number, name, account_type) VALUES
    ('1000', 'Cash',                   'asset'),
    ('1200', 'Bank Account',           'asset'),
    ('1400', 'Accounts Receivable',    'asset'),
    ('1600', 'Inventory',              'asset'),
    ('1800', 'Other Current Assets',   'asset'),
    ('3000', 'Accounts Payable',       'liability'),
    ('3500', 'VAT Payable',            'liability'),
    ('8000', 'Equity',                 'equity'),
    ('4000', 'Sales Revenue',          'revenue'),
    ('4800', 'Other Revenue',          'revenue'),
    ('5000', 'Cost of Goods Sold',     'expense'),
    ('6000', 'Salaries & Wages',       'expense'),
    ('6800', 'Office Expenses',        'expense'),
    ('7000', 'Other Expenses',         'expense')
ON CONFLICT (account_number) DO NOTHING;

-- =============================================================================
-- END OF SCHEMA
-- =============================================================================
