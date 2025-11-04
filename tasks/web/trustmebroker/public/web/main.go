package main

import (
	"bytes"
	"context"
	"database/sql"
	"flag"
	"log"
	"os"
	"os/signal"
	"path/filepath"
	"time"

	"github.com/gofiber/fiber/v2/middleware/logger"

	"github.com/gofiber/fiber/v2"
	_ "modernc.org/sqlite"
)

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
	flag.Parse()

	// db
	db, err := sql.Open("sqlite", dbPath+"?_foreign_keys=1")
	if err != nil {
		log.Fatalf("open db: %v", err)
	}
	defer db.Close()

	repo, err := NewSQLiteRepo(db)
	if err != nil {
		log.Fatalf("init repo: %v", err)
	}
	// templates
	tmpls, err := LoadTemplates(tplDir)
	if err != nil {
		log.Fatalf("load templates: %v", err)
	}

	// pdf
	pdfURL := os.Getenv("PDF_SERVICE_URL")
	if pdfURL == "" {
		log.Fatal("PDF_SERVICE_URL is required")
	}
	svc := NewService(repo, pdfURL)

	h := NewHandlers(repo, svc, tmpls)

	// serve
	app := fiber.New()

	app.Static("/assets", filepath.Clean("./assets"))

	app.Get("/", func(c *fiber.Ctx) error {
		var buf bytes.Buffer
		if err := tmpls.ExecuteTemplate(&buf, "landing.html", nil); err != nil {
			return c.Status(fiber.StatusInternalServerError).SendString("template error")
		}
		return c.Type("html").Send(buf.Bytes())
	})

	app.Use(logger.New())
	app.Get("/register", h.Register)
	app.Post("/register/step1", h.RegisterStep1)
	app.Post("/register/step2", h.RegisterStep2)
	app.Post("/register/step3", h.RegisterStep3)
	app.Get("/finish", h.Finish)
	app.Get("/agreement", h.GetAgreementPDF)

	// старт
	go func() {
		log.Printf("listening on %s", addr)
		if err := app.Listen(addr); err != nil {
			log.Printf("server stop: %v", err)
		}
	}()

	// shutdown
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	<-ctx.Done()
	stop()
	log.Println("shutting down server...")

	shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if err := app.ShutdownWithContext(shutdownCtx); err != nil {
		log.Printf("shutdown error: %v", err)
	}

	if err := db.Close(); err != nil {
		log.Printf("db close: %v", err)
	}
	log.Println("server stopped")

}

type byteBuffer struct{ b *[]byte }

func newBuffer(p *[]byte) *byteBuffer { return &byteBuffer{b: p} }
func (w *byteBuffer) Write(p []byte) (int, error) {
	*w.b = append(*w.b, p...)
	return len(p), nil
}
