package main

import (
	"database/sql"
	"errors"
	"log"
	"time"
)

type Repository interface {
	CreateApplication(app *Application) error
	GetByAgreementID(agreementID string) (*Application, error)
	UpdatePDFCreated(agreementID string) error
}

type sqliteRepo struct{ db *sql.DB }

func NewSQLiteRepo(db *sql.DB) (Repository, error) {
	log.Println("DB init: configuring PRAGMA and ensuring schema")

	if _, err := db.Exec(`PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000;`); err != nil {
		log.Printf("DB ERR pragma err=%v", err)
		return nil, err
	}

	tx, err := db.Begin()
	if err != nil {
		log.Printf("DB ERR begin tx err=%v", err)
		return nil, err
	}
	defer func() {
		if p := recover(); p != nil {
			_ = tx.Rollback()
			panic(p)
		}
	}()

	// схема без изменений
	if _, err = tx.Exec(`
CREATE TABLE IF NOT EXISTS applications (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  agreement_id TEXT UNIQUE,
  pdf_filename TEXT,
  lastname TEXT,
  firstname TEXT,
  middlename TEXT,
  birthdate TEXT,
  email TEXT,
  passport_series TEXT,
  passport_number TEXT,
  passport_issued_by TEXT,
  passport_issued_date TEXT,
  address TEXT,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  pdf_created_at DATETIME
);
CREATE INDEX IF NOT EXISTS idx_app_agreement_id ON applications(agreement_id);
`); err != nil {
		_ = tx.Rollback()
		log.Printf("DB ERR schema init err=%v", err)
		return nil, err
	}

	// очистка таблицы
	if _, err = tx.Exec(`DELETE FROM applications;`); err != nil {
		_ = tx.Rollback()
		log.Printf("DB ERR purge err=%v", err)
		return nil, err
	}

	// вставка только существующих полей
	_, err = tx.Exec(`
INSERT INTO applications (
  agreement_id, pdf_filename, lastname, firstname, middlename, birthdate, email,
  passport_series, passport_number, passport_issued_by, passport_issued_date, address
) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)
`,
		"a916728b5c3ceec5d94e2c9011a419f8",
		"agreement_a916728b5c3ceec5d94e2c9011a419f8.pdf",
		"Doe",
		"John",
		"-",
		"1994-05-05",
		"john@doe.com",
		"ctfcup{61d402f23f",
		"8f16637cff1c08f30",
		"3bebc9a9eaf19abf0",
		"56f988fcab69ef43}",
		"-",
	)
	if err != nil {
		_ = tx.Rollback()
		log.Printf("DB ERR seed insert err=%v", err)
		return nil, err
	}

	if err = tx.Commit(); err != nil {
		log.Printf("DB ERR commit err=%v", err)
		return nil, err
	}

	log.Println("DB OK schema ready (table reset with seed)")
	return &sqliteRepo{db: db}, nil
}

type Application struct {
	ID                 int64
	AgreementID        string
	PDFFileName        string
	Lastname           string
	Firstname          string
	Middlename         string
	Birthdate          string
	Email              string
	PassportSeries     string
	PassportNumber     string
	PassportIssuedBy   string
	PassportIssuedDate string
	Address            string
	CreatedAt          time.Time
	PDFCreatedAt       *time.Time
}

func (s *sqliteRepo) CreateApplication(app *Application) error {
	start := time.Now()
	log.Printf("DB INSERT start agreement_id=%s", app.AgreementID)

	res, err := s.db.Exec(`
INSERT INTO applications (
  agreement_id, pdf_filename, lastname, firstname, middlename, birthdate, email,
  passport_series, passport_number, passport_issued_by, passport_issued_date,
  address
) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)`,
		app.AgreementID, app.PDFFileName, app.Lastname, app.Firstname, app.Middlename,
		app.Birthdate, app.Email, app.PassportSeries, app.PassportNumber,
		app.PassportIssuedBy, app.PassportIssuedDate, app.Address,
	)
	if err != nil {
		log.Printf("DB ERR insert agreement_id=%s err=%v", app.AgreementID, err)
		return err
	}

	id, _ := res.LastInsertId()
	app.ID = id
	log.Printf("DB OK insert agreement_id=%s id=%d dur=%dms", app.AgreementID, id, time.Since(start).Milliseconds())
	return nil
}

func (s *sqliteRepo) GetByAgreementID(agreementID string) (*Application, error) {
	start := time.Now()
	log.Printf("DB SELECT agreement_id=%s", agreementID)

	row := s.db.QueryRow(`
SELECT id, agreement_id, pdf_filename, lastname, firstname, middlename, birthdate, email,
       passport_series, passport_number, passport_issued_by, passport_issued_date,
       address, created_at, pdf_created_at
FROM applications WHERE agreement_id = ?`, agreementID)

	var a Application
	var pdfCreatedAt sql.NullTime

	err := row.Scan(
		&a.ID, &a.AgreementID, &a.PDFFileName, &a.Lastname, &a.Firstname, &a.Middlename, &a.Birthdate, &a.Email,
		&a.PassportSeries, &a.PassportNumber, &a.PassportIssuedBy, &a.PassportIssuedDate,
		&a.Address, &a.CreatedAt, &pdfCreatedAt,
	)
	if errors.Is(err, sql.ErrNoRows) {
		log.Printf("DB WARN not_found agreement_id=%s", agreementID)
		return nil, err
	}
	if err != nil {
		log.Printf("DB ERR select agreement_id=%s err=%v", agreementID, err)
		return nil, err
	}
	if pdfCreatedAt.Valid {
		a.PDFCreatedAt = &pdfCreatedAt.Time
	}

	log.Printf("DB OK select agreement_id=%s id=%d dur=%dms", a.AgreementID, a.ID, time.Since(start).Milliseconds())
	return &a, nil
}

func (s *sqliteRepo) UpdatePDFCreated(agreementID string) error {
	start := time.Now()
	log.Printf("DB UPDATE pdf_created_at agreement_id=%s", agreementID)

	res, err := s.db.Exec(`UPDATE applications SET pdf_created_at = CURRENT_TIMESTAMP WHERE agreement_id = ?`, agreementID)
	if err != nil {
		log.Printf("DB ERR update agreement_id=%s err=%v", agreementID, err)
		return err
	}

	affected, _ := res.RowsAffected()
	log.Printf("DB OK update agreement_id=%s affected=%d dur=%dms", agreementID, affected, time.Since(start).Milliseconds())
	return nil
}
