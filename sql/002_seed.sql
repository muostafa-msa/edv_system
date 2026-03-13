-- =============================================================================
--  EDV Enterprise System — Seed / Reference Data v2.0
--  Run AFTER 001_schema.sql
-- =============================================================================

-- =============================================================================
--  CHART OF ACCOUNTS (German SKR04-inspired)
-- =============================================================================
INSERT INTO chart_of_accounts (gl_code, description, account_type, normal_balance) VALUES
-- Assets
(1000, 'Bank / Cash',                      'Asset',     'Debit'),
(1100, 'Accounts Receivable - Trade',      'Asset',     'Debit'),
(1200, 'Advance Payments to Vendors',      'Asset',     'Debit'),
(1400, 'Finished Goods Inventory',         'Asset',     'Debit'),
(1410, 'Raw Materials Inventory',          'Asset',     'Debit'),
(1500, 'Property Plant and Equipment',     'Asset',     'Debit'),
-- Liabilities
(2000, 'Accounts Payable - Trade',         'Liability', 'Credit'),
(2100, 'VAT Payable',                      'Liability', 'Credit'),
(2200, 'Accrued Salaries',                 'Liability', 'Credit'),
(2300, 'Social Security Payable',          'Liability', 'Credit'),
(2400, 'Customer Advance Payments',        'Liability', 'Credit'),
-- Equity
(3000, 'Share Capital',                    'Equity',    'Credit'),
(3100, 'Retained Earnings',                'Equity',    'Credit'),
(3200, 'Current Year Profit/Loss',         'Equity',    'Credit'),
-- Revenue
(4000, 'Revenue - Product Sales',          'Revenue',   'Credit'),
(4100, 'Revenue - Services',               'Revenue',   'Credit'),
(4200, 'Other Operating Revenue',          'Revenue',   'Credit'),
-- Expenses
(5000, 'Cost of Goods Sold',               'Expense',   'Debit'),
(5100, 'Raw Material Consumption',         'Expense',   'Debit'),
(6000, 'Salaries and Wages',               'Expense',   'Debit'),
(6100, 'Employer Social Security',         'Expense',   'Debit'),
(6200, 'Travel and Entertainment',         'Expense',   'Debit'),
(6300, 'Office and Administration',        'Expense',   'Debit'),
(6400, 'Depreciation',                     'Expense',   'Debit'),
(6500, 'Rent and Utilities',               'Expense',   'Debit'),
(7000, 'Interest Expense',                 'Expense',   'Debit'),
(7100, 'Bank Charges',                     'Expense',   'Debit'),
(8000, 'Income Tax Expense',               'Expense',   'Debit')
ON CONFLICT (gl_code) DO NOTHING;

-- =============================================================================
--  COST CENTERS
-- =============================================================================
INSERT INTO cost_centers (code, description) VALUES
('CC-SALES',  'Sales Department'),
('CC-PROD',   'Production / Manufacturing'),
('CC-IT',     'Information Technology'),
('CC-HR',     'Human Resources'),
('CC-MGMT',   'General Management'),
('CC-LOG',    'Logistics / Warehouse')
ON CONFLICT (code) DO NOTHING;

-- =============================================================================
--  WAREHOUSES
-- =============================================================================
INSERT INTO warehouses (code, description, location) VALUES
('WH-MAIN',   'Main Warehouse',     'Building A, Zone 1'),
('WH-RETURN', 'Returns Warehouse',  'Building B, Zone 3'),
('WH-QA',     'QA / Quarantine',    'Building A, Zone 2')
ON CONFLICT (code) DO NOTHING;

-- =============================================================================
--  DEMO CUSTOMERS
-- =============================================================================
INSERT INTO customers (customer_number, company_name, contact_name, email, city, country, credit_limit, payment_terms, gl_ar_account) VALUES
('C000001', 'Müller GmbH',           'Hans Müller',   'hans@mueller.de',       'München',   'DE', 50000, 30, 1100),
('C000002', 'Weber Industrietechnik','Anna Weber',    'anna@weber-it.de',      'Hamburg',   'DE', 75000, 45, 1100),
('C000003', 'Schmidt & Partner',     'Klaus Schmidt', 'k.schmidt@sp.de',       'Berlin',    'DE', 30000, 14, 1100),
('C000004', 'Bauer Electronics',     'Maria Bauer',   'maria@bauerelec.de',    'Stuttgart', 'DE', 60000, 30, 1100),
('C000005', 'Fischer Logistics',     'Peter Fischer', 'p.fischer@fl.de',       'Köln',      'DE', 25000, 60, 1100)
ON CONFLICT (customer_number) DO NOTHING;

SELECT SETVAL('seq_customer', 5, true);

-- =============================================================================
--  DEMO VENDORS
-- =============================================================================
INSERT INTO vendors (vendor_number, company_name, contact_name, email, city, country, payment_terms, gl_ap_account) VALUES
('V000001', 'Roh Material AG',       'Thomas Roh',    'thomas@rohmaterial.de', 'Frankfurt', 'DE', 30, 2000),
('V000002', 'Elektro Zulieferer GmbH','Lisa Zulieferer','lisa@ezg.de',         'Nürnberg',  'DE', 45, 2000),
('V000003', 'Verpackung Pro KG',     'Max Verpackung','max@verpro.de',         'Dortmund',  'DE', 14, 2000)
ON CONFLICT (vendor_number) DO NOTHING;

SELECT SETVAL('seq_vendor', 3, true);

-- =============================================================================
--  DEMO MATERIALS
-- =============================================================================
INSERT INTO materials (material_number, description, unit_of_measure, material_type, base_price, min_stock_level, reorder_point, gl_inventory_account) VALUES
('MAT000001', 'Industrial Gear Assembly A12',  'EA', 'FERT', 245.00, 10,  5,  1400),
('MAT000002', 'Control Panel Unit CP-500',     'EA', 'FERT', 1850.00, 5,  3,  1400),
('MAT000003', 'Hydraulic Valve HV-200',        'EA', 'HALB', 320.00, 20, 10, 1400),
('MAT000004', 'Steel Sheet 3mm (1x2m)',        'EA', 'ROH',  45.00, 100, 50, 1410),
('MAT000005', 'Electronic Control Board ECB3', 'EA', 'HALB', 780.00, 15,  8, 1400),
('MAT000006', 'Bearing Set BS-10 (pack of 4)', 'PK', 'ROH',  28.50, 200, 80, 1410),
('MAT000007', 'Consulting Service (hourly)',   'H',  'DIEN', 150.00, 0,   0, NULL),
('MAT000008', 'Maintenance Kit MK-Pro',        'EA', 'HAWA', 92.00, 30,  15, 1400)
ON CONFLICT (material_number) DO NOTHING;

SELECT SETVAL('seq_material', 8, true);

-- =============================================================================
--  INITIAL STOCK (inventory_ledger)
-- =============================================================================
INSERT INTO inventory_ledger (material_id, warehouse_id, movement_type, quantity, unit_cost, movement_date, reference_doc)
SELECT
    m.id,
    w.id,
    'OPENING',
    CASE m.material_number
        WHEN 'MAT000001' THEN 25
        WHEN 'MAT000002' THEN 8
        WHEN 'MAT000003' THEN 45
        WHEN 'MAT000004' THEN 250
        WHEN 'MAT000005' THEN 20
        WHEN 'MAT000006' THEN 300
        WHEN 'MAT000008' THEN 50
        ELSE 0
    END,
    m.base_price,
    CURRENT_DATE,
    'OPENING_BALANCE'
FROM materials m, warehouses w
WHERE w.code = 'WH-MAIN'
  AND m.material_type != 'DIEN';

-- =============================================================================
--  PRICING CONDITIONS (PR00 = base price)
-- =============================================================================
INSERT INTO pricing_conditions (condition_type, material_id, valid_from, valid_to, amount, calc_type)
SELECT 'PR00', id, CURRENT_DATE, '2099-12-31', base_price, 'FIXED'
FROM materials
WHERE base_price > 0
ON CONFLICT DO NOTHING;

-- Customer-specific discount for Müller GmbH (5%)
INSERT INTO pricing_conditions (condition_type, material_id, customer_id, valid_from, valid_to, amount, calc_type)
SELECT 'K004', m.id, c.id, CURRENT_DATE, '2099-12-31', -5.0, 'PCT'
FROM materials m, customers c
WHERE c.customer_number = 'C000001'
ON CONFLICT DO NOTHING;

-- =============================================================================
--  DEMO EMPLOYEES
-- =============================================================================
INSERT INTO employees (employee_number, first_name, last_name, department, position, hire_date, salary, cost_center_id)
SELECT
    emp.number, emp.first_name, emp.last_name, emp.dept, emp.pos,
    emp.hire_date::DATE, emp.salary,
    cc.id
FROM (VALUES
    ('EMP000001', 'Anna',    'Schneider', 'Sales',      'Sales Manager',        '2019-03-01', 72000, 'CC-SALES'),
    ('EMP000002', 'Bernd',   'Hoffmann',  'Production', 'Production Lead',      '2018-07-15', 65000, 'CC-PROD'),
    ('EMP000003', 'Claudia', 'Wagner',    'IT',         'System Administrator', '2021-01-10', 68000, 'CC-IT'),
    ('EMP000004', 'Dieter',  'Braun',     'HR',         'HR Specialist',        '2020-09-01', 58000, 'CC-HR'),
    ('EMP000005', 'Eva',     'Schwarz',   'Management', 'CEO',                  '2015-01-01', 120000,'CC-MGMT')
) AS emp(number, first_name, last_name, dept, pos, hire_date, salary, cc_code)
JOIN cost_centers cc ON cc.code = emp.cc_code
ON CONFLICT (employee_number) DO NOTHING;

SELECT SETVAL('seq_employee', 5, true);
