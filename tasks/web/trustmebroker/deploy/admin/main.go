package main

import (
	"database/sql"
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	_ "modernc.org/sqlite"
)

const pageSize = 10

type Server struct {
	db   *sql.DB
	tmpl *template.Template
	tz   *time.Location
	now  func() time.Time
}

func main() {
	addr := ":" + os.Getenv("PORT")
	if addr == ":" {
		addr = ":8080"
	}

	dbPath := os.Getenv("DB_PATH")
	if dbPath == "" {
		dbPath = "./db/reg.db"
	}

	tplDir := "templates"
	log.Printf("admin boot: addr=%s db=%s tplDir=%s", addr, dbPath, tplDir)

	t0 := time.Now()
	tmpl, err := template.ParseFiles(
		filepath.Join(tplDir, "main.html"),
		filepath.Join(tplDir, "agreements.html"),
	)
	if err != nil {
		log.Fatalf("parse templates: %v", err)
	}
	log.Printf("templates loaded in %dms", time.Since(t0).Milliseconds())

	// --- подключение к БД с ретраями ---
	var db *sql.DB
	for {
		dsn := fmt.Sprintf("file:%s?mode=ro&cache=shared&_pragma=busy_timeout(3000)", filepath.ToSlash(dbPath))
		db, err = sql.Open("sqlite", dsn)
		if err != nil {
			log.Printf("DB WARN open failed: %v (retry in 1s)", err)
			time.Sleep(1 * time.Second)
			continue
		}

		if pingErr := db.Ping(); pingErr != nil {
			log.Printf("DB WARN ping failed: %v (retry in 1s)", pingErr)
			_ = db.Close()
			time.Sleep(1 * time.Second)
			continue
		}

		log.Printf("DB connected (ro)")
		break
	}
	// --- конец подключения ---

	loc, _ := time.LoadLocation("Europe/Helsinki")
	s := &Server{db: db, tmpl: tmpl, tz: loc, now: time.Now}

	mux := http.NewServeMux()
	mux.HandleFunc("/", s.handleRoot)
	mux.HandleFunc("/agreements", s.handleAgreements)

	log.Printf("admin listening on %s (db=%s ro)", addr, dbPath)
	log.Fatal(http.ListenAndServe(addr, logMiddleware(mux)))
}

func (s *Server) handleRoot(w http.ResponseWriter, r *http.Request) {
	start := time.Now()
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		log.Printf("ERR / method=%s 405", r.Method)
		return
	}
	target := time.Date(2025, 11, 4, 12, 0, 0, 0, s.tz)
	now := s.now().In(s.tz)

	var days, hours int
	if now.After(target) {
		days, hours = 0, 0
	} else {
		diff := target.Sub(now)
		days = int(diff.Hours()) / 24
		hours = int(diff.Hours()) % 24
	}

	if err := s.tmpl.ExecuteTemplate(w, "main.html", map[string]any{"days": days, "hours": hours}); err != nil {
		log.Printf("ERR render main.html err=%v", err)
		http.Error(w, "template error", http.StatusInternalServerError)
		return
	}
	log.Printf("OK  GET / dur=%dms days=%d hours=%d", time.Since(start).Milliseconds(), days, hours)
}

func (s *Server) handleAgreements(w http.ResponseWriter, r *http.Request) {
	start := time.Now()
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		log.Printf("ERR /agreements method=%s 405", r.Method)
		return
	}

	page := parsePage(r.URL.Query().Get("page"))
	offset := (page - 1) * pageSize
	log.Printf("REQ GET /agreements page=%d offset=%d limit=%d", page, offset, pageSize)

	var (
		total int
		rows  []row
		err   error
	)

	if s.db == nil {
		err = fmt.Errorf("нет подключения к базе")
		log.Printf("ERR /agreements no-db")
	} else {

		t1 := time.Now()
		total, err = s.count()
		if err != nil {
			log.Printf("ERR db.count err=%v", err)
		} else {
			log.Printf("OK  db.count total=%d dur=%dms", total, time.Since(t1).Milliseconds())
		}

		if err == nil {
			t2 := time.Now()
			rows, err = s.list(pageSize, offset)
			if err != nil {
				log.Printf("ERR db.list limit=%d offset=%d err=%v", pageSize, offset, err)
			} else {
				log.Printf("OK  db.list rows=%d dur=%dms", len(rows), time.Since(t2).Milliseconds())
			}
		}
	}

	data := map[string]any{
		"page":       page,
		"agreements": toView(rows),
		"from":       ifThen(total > 0, offset+1, 0),
		"to":         ifThen(total > 0, offset+len(rows), 0),
		"total":      total,
		"has_prev":   page > 1,
		"has_next":   offset+len(rows) < total,
		"prev_page":  max(1, page-1),
		"next_page":  page + 1,
		"load_err":   errString(err),
	}

	if tmplErr := s.tmpl.ExecuteTemplate(w, "agreements.html", data); tmplErr != nil {
		log.Printf("ERR render agreements.html err=%v", tmplErr)
		http.Error(w, "template error", http.StatusInternalServerError)
		return
	}

	log.Printf("OK  GET /agreements page=%d showed=%d..%d total=%d err=%v dur=%dms",
		page, data["from"], data["to"], total, err, time.Since(start).Milliseconds())
}

type row struct {
	ID          int64
	AgreementID string
	FirstName   string
	LastName    string
	MiddleName  string
	Email       string
}

func (s *Server) count() (int, error) {
	var c int
	err := s.db.QueryRow(`SELECT COUNT(*) FROM applications`).Scan(&c)
	return c, err
}

func (s *Server) list(limit, offset int) ([]row, error) {
	q := `
SELECT id, agreement_id, firstname, lastname, middlename, email
FROM applications
ORDER BY id ASC
LIMIT ? OFFSET ?;
`
	rows, err := s.db.Query(q, limit, offset)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var out []row
	for rows.Next() {
		var r row
		if err := rows.Scan(&r.ID, &r.AgreementID, &r.FirstName, &r.LastName, &r.MiddleName, &r.Email); err != nil {
			return nil, err
		}
		out = append(out, r)
	}
	return out, rows.Err()
}

type view struct {
	ID              int64
	Fullname        string
	Email           string
	AgreementNumber string
}

func toView(rs []row) []view {
	out := make([]view, 0, len(rs))
	for _, r := range rs {
		full := strings.TrimSpace(strings.Join([]string{r.LastName, r.FirstName, r.MiddleName}, " "))
		out = append(out, view{
			ID:              r.ID,
			Fullname:        full,
			Email:           r.Email,
			AgreementNumber: r.AgreementID,
		})
	}
	return out
}

func parsePage(s string) int {
	n, err := strconv.Atoi(s)
	if err != nil || n < 1 || n > 10000 {
		return 1
	}
	return n
}

func ifThen[T any](cond bool, a, b T) T {
	if cond {
		return a
	}
	return b
}
func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}
func errString(err error) string {
	if err == nil {
		return ""
	}
	return err.Error()
}

func logMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		st := time.Now()
		next.ServeHTTP(w, r)
		log.Printf("%s %s %dms ua=%q ip=%s", r.Method, r.URL.String(), time.Since(st).Milliseconds(), r.UserAgent(), clientIP(r))
	})
}

func clientIP(r *http.Request) string {
	if xff := r.Header.Get("X-Forwarded-For"); xff != "" {
		parts := strings.Split(xff, ",")
		return strings.TrimSpace(parts[0])
	}
	if xr := r.Header.Get("X-Real-IP"); xr != "" {
		return xr
	}
	return r.RemoteAddr
}
