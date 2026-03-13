-- =============================================================================
--  EDV Enterprise System — PostgreSQL Schema v2.0
--  Universal Journal Architecture (ACDOCA-inspired)
--  Compatible with PostgreSQL 14+
-- =============================================================================

-- Enable extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- =============================================================================
--  DIMENSION TABLES
-- =============================================================================

-- ── Users & RBAC ──────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS users (
    id             SERIAL PRIMARY KEY,
    username       VARCHAR(64)  NOT NULL UNIQUE,
    display_name   VARCHAR(128) NOT NULL,
    password_hash  CHAR(64)     NOT NULL,   -- SHA-256 hex
    role           VARCHAR(20)  NOT NULL DEFAULT 'Staff'
                   CHECK (role IN ('Admin','Executive','Manager','Staff')),
    email          VARCHAR(256),
    is_active      BOOLEAN      NOT NULL DEFAULT TRUE,
    last_login     TIMESTAMPTZ,
    created_at     TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    updated_at     TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

-- ── Chart of Accounts ────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS chart_of_accounts (
    id             SERIAL PRIMARY KEY,
    gl_code        INTEGER      NOT NULL UNIQUE,   -- e.g. 1000, 4100
    description    VARCHAR(256) NOT NULL,
    account_type   VARCHAR(20)  NOT NULL            -- Asset, Liability, Equity, Revenue, Expense
                   CHECK (account_type IN ('Asset','Liability','Equity','Revenue','Expense')),
    normal_balance VARCHAR(10)  NOT NULL DEFAULT 'Debit'
                   CHECK (normal_balance IN ('Debit','Credit')),
    is_active      BOOLEAN      NOT NULL DEFAULT TRUE
);

-- ── Cost Centers ──────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS cost_centers (
    id             SERIAL PRIMARY KEY,
    code           VARCHAR(20)  NOT NULL UNIQUE,
    description    VARCHAR(256) NOT NULL,
    manager_user_id INT REFERENCES users(id),
    is_active      BOOLEAN NOT NULL DEFAULT TRUE
);

-- ── Customers ─────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS customers (
    id             SERIAL PRIMARY KEY,
    customer_number VARCHAR(20) NOT NULL UNIQUE,  -- C000001
    company_name   VARCHAR(256) NOT NULL,
    contact_name   VARCHAR(128),
    email          VARCHAR(256),
    phone          VARCHAR(64),
    address_line1  VARCHAR(256),
    address_line2  VARCHAR(256),
    city           VARCHAR(128),
    postal_code    VARCHAR(20),
    country        CHAR(2) DEFAULT 'DE',
    tax_id         VARCHAR(64),
    credit_limit   NUMERIC(15,2) DEFAULT 0,
    payment_terms  INTEGER DEFAULT 30,          -- days
    gl_ar_account  INTEGER REFERENCES chart_of_accounts(gl_code),
    is_active      BOOLEAN NOT NULL DEFAULT TRUE,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE SEQUENCE IF NOT EXISTS seq_customer START 1;

-- ── Vendors ───────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS vendors (
    id             SERIAL PRIMARY KEY,
    vendor_number  VARCHAR(20) NOT NULL UNIQUE,   -- V000001
    company_name   VARCHAR(256) NOT NULL,
    contact_name   VARCHAR(128),
    email          VARCHAR(256),
    phone          VARCHAR(64),
    address_line1  VARCHAR(256),
    city           VARCHAR(128),
    country        CHAR(2) DEFAULT 'DE',
    tax_id         VARCHAR(64),
    payment_terms  INTEGER DEFAULT 30,
    gl_ap_account  INTEGER REFERENCES chart_of_accounts(gl_code),
    is_active      BOOLEAN NOT NULL DEFAULT TRUE,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE SEQUENCE IF NOT EXISTS seq_vendor START 1;

-- ── Materials / Products ──────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS materials (
    id             SERIAL PRIMARY KEY,
    material_number VARCHAR(40) NOT NULL UNIQUE,  -- MAT000001
    description    VARCHAR(512) NOT NULL,
    unit_of_measure VARCHAR(10) NOT NULL DEFAULT 'EA',
    material_type  VARCHAR(20) NOT NULL DEFAULT 'FERT'
                   CHECK (material_type IN ('FERT','HALB','ROH','DIEN','HAWA')),
    base_price     NUMERIC(15,4) NOT NULL DEFAULT 0,
    currency       CHAR(3) DEFAULT 'EUR',
    weight_kg      NUMERIC(10,3),
    min_stock_level INT DEFAULT 0,
    max_stock_level INT,
    reorder_point  INT DEFAULT 0,
    gl_inventory_account INT REFERENCES chart_of_accounts(gl_code),
    is_active      BOOLEAN NOT NULL DEFAULT TRUE,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE SEQUENCE IF NOT EXISTS seq_material START 1;

-- ── Warehouses ────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS warehouses (
    id             SERIAL PRIMARY KEY,
    code           VARCHAR(10) NOT NULL UNIQUE,
    description    VARCHAR(256) NOT NULL,
    location       VARCHAR(512),
    is_active      BOOLEAN NOT NULL DEFAULT TRUE
);

-- ── Employees ────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS employees (
    id             SERIAL PRIMARY KEY,
    employee_number VARCHAR(20) NOT NULL UNIQUE,  -- EMP000001
    first_name     VARCHAR(128) NOT NULL,
    last_name      VARCHAR(128) NOT NULL,
    email          VARCHAR(256),
    phone          VARCHAR(64),
    department     VARCHAR(128),
    position       VARCHAR(128),
    hire_date      DATE,
    salary         NUMERIC(12,2),
    currency       CHAR(3) DEFAULT 'EUR',
    cost_center_id INT REFERENCES cost_centers(id),
    user_id        INT REFERENCES users(id),
    is_active      BOOLEAN NOT NULL DEFAULT TRUE,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE SEQUENCE IF NOT EXISTS seq_employee START 1;

-- =============================================================================
--  UNIVERSAL JOURNAL (FACT TABLE — ACDOCA Pattern)
-- =============================================================================

-- ── Document header ───────────────────────────────────────────────────────────
CREATE SEQUENCE IF NOT EXISTS seq_journal_document START 1;

CREATE TABLE IF NOT EXISTS journal_document_header (
    document_number   VARCHAR(30) PRIMARY KEY,   -- e.g. SA-2025-000042
    document_type     VARCHAR(5)  NOT NULL,       -- SA, DR, KR, WE, WA, PR, HR
    posting_date      DATE        NOT NULL,
    document_date     DATE        NOT NULL,
    reference         VARCHAR(128),
    header_text       VARCHAR(512),
    currency          CHAR(3)     NOT NULL DEFAULT 'EUR',
    is_reversed       BOOLEAN     NOT NULL DEFAULT FALSE,
    reversal_doc_num  VARCHAR(30),
    posted_by_user_id INT         REFERENCES users(id),
    created_at        TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ── Universal Journal line items (ACDOCA-style fact table) ───────────────────
CREATE TABLE IF NOT EXISTS fact_universal_journal (
    id                BIGSERIAL   PRIMARY KEY,
    document_number   VARCHAR(30) NOT NULL REFERENCES journal_document_header(document_number),
    posting_date      DATE        NOT NULL,
    gl_account        INTEGER     NOT NULL REFERENCES chart_of_accounts(gl_code),
    amount            NUMERIC(18,4) NOT NULL,    -- Positive=Debit, Negative=Credit
    currency          CHAR(3)     NOT NULL DEFAULT 'EUR',
    cost_center       VARCHAR(20),
    profit_center     VARCHAR(20),
    line_text         VARCHAR(512),

    -- Reference dimensions (all nullable — depend on document type)
    customer_id       INT REFERENCES customers(id),
    vendor_id         INT REFERENCES vendors(id),
    material_id       INT REFERENCES materials(id),
    employee_id       INT REFERENCES employees(id),
    sales_order_id    INT,                        -- FK to sales_orders
    purchase_order_id INT,                        -- FK to purchase_orders
    warehouse_id      INT REFERENCES warehouses(id),

    created_at        TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Indexes for common reporting queries
CREATE INDEX IF NOT EXISTS idx_fuj_document   ON fact_universal_journal(document_number);
CREATE INDEX IF NOT EXISTS idx_fuj_date       ON fact_universal_journal(posting_date);
CREATE INDEX IF NOT EXISTS idx_fuj_gl         ON fact_universal_journal(gl_account);
CREATE INDEX IF NOT EXISTS idx_fuj_customer   ON fact_universal_journal(customer_id) WHERE customer_id IS NOT NULL;
CREATE INDEX IF NOT EXISTS idx_fuj_material   ON fact_universal_journal(material_id) WHERE material_id IS NOT NULL;

-- =============================================================================
--  LOGISTICS TABLES (MM / WM)
-- =============================================================================

CREATE TABLE IF NOT EXISTS inventory_ledger (
    id              BIGSERIAL    PRIMARY KEY,
    material_id     INT          NOT NULL REFERENCES materials(id),
    warehouse_id    INT          NOT NULL REFERENCES warehouses(id),
    movement_type   VARCHAR(20)  NOT NULL,   -- GR_PO, GI_SO, ADJUST, RETURN
    quantity        NUMERIC(15,4) NOT NULL,   -- + in, - out
    unit_cost       NUMERIC(15,4),
    journal_doc_num VARCHAR(30)  REFERENCES journal_document_header(document_number),
    reference_doc   VARCHAR(64),
    movement_date   DATE         NOT NULL DEFAULT CURRENT_DATE,
    created_by_user INT          REFERENCES users(id),
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE OR REPLACE VIEW stock_levels AS
    SELECT
        material_id,
        warehouse_id,
        SUM(quantity)                AS quantity_on_hand,
        m.description                AS material_description,
        m.unit_of_measure,
        m.min_stock_level,
        m.reorder_point,
        w.code                       AS warehouse_code,
        w.description                AS warehouse_description
    FROM inventory_ledger il
    JOIN materials  m ON m.id = il.material_id
    JOIN warehouses w ON w.id = il.warehouse_id
    GROUP BY il.material_id, il.warehouse_id,
             m.description, m.unit_of_measure,
             m.min_stock_level, m.reorder_point,
             w.code, w.description;

-- =============================================================================
--  SALES & DISTRIBUTION (SD)
-- =============================================================================

CREATE SEQUENCE IF NOT EXISTS seq_sales_order START 1000;

CREATE TABLE IF NOT EXISTS sales_orders (
    id              SERIAL       PRIMARY KEY,
    order_number    VARCHAR(20)  NOT NULL UNIQUE,   -- SO-001000
    customer_id     INT          NOT NULL REFERENCES customers(id),
    order_date      DATE         NOT NULL DEFAULT CURRENT_DATE,
    requested_date  DATE,
    status          VARCHAR(20)  NOT NULL DEFAULT 'draft'
                    CHECK (status IN ('draft','confirmed','picking','shipped','invoiced','cancelled')),
    currency        CHAR(3)      DEFAULT 'EUR',
    net_amount      NUMERIC(15,2) NOT NULL DEFAULT 0,
    tax_amount      NUMERIC(15,2) NOT NULL DEFAULT 0,
    total_gross     NUMERIC(15,2) NOT NULL DEFAULT 0,
    journal_doc_num VARCHAR(30)  REFERENCES journal_document_header(document_number),
    notes           TEXT,
    created_by_user INT          REFERENCES users(id),
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS sales_order_items (
    id              SERIAL       PRIMARY KEY,
    sales_order_id  INT          NOT NULL REFERENCES sales_orders(id) ON DELETE CASCADE,
    line_number     SMALLINT     NOT NULL,
    material_id     INT          NOT NULL REFERENCES materials(id),
    quantity        NUMERIC(15,4) NOT NULL,
    unit_of_measure VARCHAR(10),
    unit_price      NUMERIC(15,4) NOT NULL,
    discount_pct    NUMERIC(5,2)  DEFAULT 0,
    net_amount      NUMERIC(15,2) NOT NULL,
    tax_rate        NUMERIC(5,2)  DEFAULT 19.0,     -- VAT %
    UNIQUE (sales_order_id, line_number)
);

-- SD Pricing Conditions (SAP condition technique)
CREATE TABLE IF NOT EXISTS pricing_conditions (
    id              SERIAL       PRIMARY KEY,
    condition_type  VARCHAR(10)  NOT NULL,    -- PR00, K004, MWST, etc.
    material_id     INT          REFERENCES materials(id),
    customer_id     INT          REFERENCES customers(id),
    valid_from      DATE         NOT NULL,
    valid_to        DATE,
    amount          NUMERIC(15,4) NOT NULL,
    calc_type       VARCHAR(10)  NOT NULL DEFAULT 'FIXED'
                    CHECK (calc_type IN ('FIXED','PCT','QTY')),
    currency        CHAR(3)      DEFAULT 'EUR',
    is_active       BOOLEAN      NOT NULL DEFAULT TRUE
);

-- =============================================================================
--  PURCHASING (MM)
-- =============================================================================

CREATE SEQUENCE IF NOT EXISTS seq_purchase_order START 4500;

CREATE TABLE IF NOT EXISTS purchase_orders (
    id              SERIAL       PRIMARY KEY,
    po_number       VARCHAR(20)  NOT NULL UNIQUE,   -- PO-004500
    vendor_id       INT          NOT NULL REFERENCES vendors(id),
    po_date         DATE         NOT NULL DEFAULT CURRENT_DATE,
    status          VARCHAR(20)  NOT NULL DEFAULT 'draft'
                    CHECK (status IN ('draft','sent','partial','received','invoiced','cancelled')),
    currency        CHAR(3)      DEFAULT 'EUR',
    net_amount      NUMERIC(15,2) NOT NULL DEFAULT 0,
    tax_amount      NUMERIC(15,2) NOT NULL DEFAULT 0,
    total_gross     NUMERIC(15,2) NOT NULL DEFAULT 0,
    journal_doc_num VARCHAR(30)  REFERENCES journal_document_header(document_number),
    notes           TEXT,
    created_by_user INT          REFERENCES users(id),
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS purchase_order_items (
    id              SERIAL       PRIMARY KEY,
    purchase_order_id INT        NOT NULL REFERENCES purchase_orders(id) ON DELETE CASCADE,
    line_number     SMALLINT     NOT NULL,
    material_id     INT          NOT NULL REFERENCES materials(id),
    quantity        NUMERIC(15,4) NOT NULL,
    unit_of_measure VARCHAR(10),
    unit_price      NUMERIC(15,4) NOT NULL,
    quantity_received NUMERIC(15,4) DEFAULT 0,
    UNIQUE (purchase_order_id, line_number)
);

-- =============================================================================
--  PRODUCTION PLANNING (PP)
-- =============================================================================

CREATE TABLE IF NOT EXISTS bill_of_materials (
    id              SERIAL       PRIMARY KEY,
    parent_material_id INT       NOT NULL REFERENCES materials(id),
    component_material_id INT    NOT NULL REFERENCES materials(id),
    quantity        NUMERIC(15,4) NOT NULL,
    unit_of_measure VARCHAR(10),
    UNIQUE (parent_material_id, component_material_id)
);

CREATE TABLE IF NOT EXISTS production_orders (
    id              SERIAL       PRIMARY KEY,
    order_number    VARCHAR(20)  NOT NULL UNIQUE,
    material_id     INT          NOT NULL REFERENCES materials(id),
    planned_qty     NUMERIC(15,4) NOT NULL,
    produced_qty    NUMERIC(15,4) NOT NULL DEFAULT 0,
    start_date      DATE,
    end_date        DATE,
    status          VARCHAR(20)  NOT NULL DEFAULT 'planned'
                    CHECK (status IN ('planned','released','in_progress','completed','cancelled')),
    warehouse_id    INT          REFERENCES warehouses(id),
    created_by_user INT          REFERENCES users(id),
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

-- =============================================================================
--  HUMAN RESOURCES (HR)
-- =============================================================================

CREATE TABLE IF NOT EXISTS payroll_runs (
    id              SERIAL       PRIMARY KEY,
    pay_period      DATE         NOT NULL,           -- First day of pay period
    pay_period_end  DATE         NOT NULL,
    status          VARCHAR(20)  NOT NULL DEFAULT 'draft'
                    CHECK (status IN ('draft','calculated','approved','posted','cancelled')),
    journal_doc_num VARCHAR(30)  REFERENCES journal_document_header(document_number),
    run_by_user_id  INT          REFERENCES users(id),
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS payroll_lines (
    id              SERIAL       PRIMARY KEY,
    payroll_run_id  INT          NOT NULL REFERENCES payroll_runs(id) ON DELETE CASCADE,
    employee_id     INT          NOT NULL REFERENCES employees(id),
    gross_pay       NUMERIC(12,2) NOT NULL DEFAULT 0,
    tax_deduction   NUMERIC(12,2) NOT NULL DEFAULT 0,
    social_security NUMERIC(12,2) NOT NULL DEFAULT 0,
    net_pay         NUMERIC(12,2) NOT NULL DEFAULT 0,
    currency        CHAR(3)      DEFAULT 'EUR'
);

-- =============================================================================
--  AUDIT TRIGGER (updated_at auto-update)
-- =============================================================================

CREATE OR REPLACE FUNCTION update_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER trg_customers_updated_at
    BEFORE UPDATE ON customers
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

CREATE OR REPLACE TRIGGER trg_sales_orders_updated_at
    BEFORE UPDATE ON sales_orders
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

CREATE OR REPLACE TRIGGER trg_users_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();
