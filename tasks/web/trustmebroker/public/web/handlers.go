package main

import (
	"bytes"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"html/template"
	"log"
	"path/filepath"
	"strings"
	"time"

	"github.com/gofiber/fiber/v2"
)

type Handlers struct {
	repo Repository
	svc  *Service
	tmpl *template.Template
}

func NewHandlers(r Repository, s *Service, t *template.Template) *Handlers {
	return &Handlers{repo: r, svc: s, tmpl: t}
}

func LoadTemplates(dir string) (*template.Template, error) {
	return template.ParseGlob(filepath.Join(dir, "*.html"))
}

func newRID() string {
	var b [6]byte
	_, _ = rand.Read(b[:])
	return hex.EncodeToString(b[:])
}

func dumpMap(m map[string]string) string {
	if m == nil {
		return "{}"
	}
	b, _ := json.Marshal(m)
	return string(b)
}

func (h *Handlers) Register(c *fiber.Ctx) error {
	if c.Method() != fiber.MethodGet {
		return c.SendStatus(fiber.StatusMethodNotAllowed)
	}
	rid := newRID()
	start := time.Now()
	log.Printf("REQ %s GET /register ip=%s ua=%q", rid, c.IP(), string(c.Request().Header.UserAgent()))

	var buf bytes.Buffer
	if err := h.tmpl.ExecuteTemplate(&buf, "register.html", nil); err != nil {
		log.Printf("ERR %s tpl.register err=%v", rid, err)
		return c.Status(fiber.StatusInternalServerError).SendString("template error")
	}

	log.Printf("OK  %s GET /register dur=%dms bytes=%d", rid, time.Since(start).Milliseconds(), buf.Len())
	return c.Type("html").Send(buf.Bytes())
}

func (h *Handlers) RegisterStep1(c *fiber.Ctx) error {
	if c.Method() != fiber.MethodPost {
		return c.SendStatus(fiber.StatusMethodNotAllowed)
	}
	rid := newRID()
	start := time.Now()

	var in map[string]string
	if err := c.BodyParser(&in); err != nil {
		log.Printf("ERR %s POST /register/step1 bad_json ip=%s sz=%d err=%v", rid, c.IP(), len(c.Body()), err)
		return c.Status(fiber.StatusBadRequest).JSON(fiber.Map{"ok": false, "error": "bad json"})
	}
	log.Printf("REQ %s POST /register/step1 ip=%s sz=%d body=%s", rid, c.IP(), len(c.Body()), dumpMap(in))

	if errs := validateStep1(in); len(errs) > 0 {
		log.Printf("VAL %s step1 invalid errs=%v", rid, errs)
		return c.Status(fiber.StatusUnprocessableEntity).JSON(fiber.Map{"ok": false, "errors": errs})
	}
	log.Printf("OK  %s step1 dur=%dms", rid, time.Since(start).Milliseconds())
	return c.JSON(fiber.Map{"ok": true})
}

func (h *Handlers) RegisterStep2(c *fiber.Ctx) error {
	if c.Method() != fiber.MethodPost {
		return c.SendStatus(fiber.StatusMethodNotAllowed)
	}
	rid := newRID()
	start := time.Now()

	var in map[string]string
	if err := c.BodyParser(&in); err != nil {
		log.Printf("ERR %s POST /register/step2 bad_json ip=%s sz=%d err=%v", rid, c.IP(), len(c.Body()), err)
		return c.Status(fiber.StatusBadRequest).JSON(fiber.Map{"ok": false, "error": "bad json"})
	}
	log.Printf("REQ %s POST /register/step2 ip=%s sz=%d body=%s", rid, c.IP(), len(c.Body()), dumpMap(in))

	var allErrs []string
	allErrs = append(allErrs, validateStep1(in)...)
	allErrs = append(allErrs, validateStep2(in)...)
	if len(allErrs) > 0 {
		log.Printf("VAL %s step2 invalid errs=%v", rid, allErrs)
		return c.Status(fiber.StatusUnprocessableEntity).JSON(fiber.Map{"ok": false, "errors": allErrs})
	}
	for k := range in {
		in[k] = sanitize(in[k])
	}

	in["agreement_id"] = GenerateAgreementID()
	in["pdf_filename"] = "agreement_" + in["agreement_id"] + ".pdf"
	agreementID := strings.TrimSpace(in["agreement_id"])
	log.Printf("INF %s step2 ids agreement_id=%s pdf=%s", rid, agreementID, in["pdf_filename"])

	pdfStart := time.Now()
	path, err := h.svc.GeneratePDFForApplication(&in)
	if err != nil {
		log.Printf("ERR %s pdf_fail agreement_id=%s err=%v", rid, agreementID, err)
	} else {
		log.Printf("OK  %s pdf_ok agreement_id=%s path=%s dur=%dms", rid, agreementID, path, time.Since(pdfStart).Milliseconds())
	}

	app := &Application{
		AgreementID:        agreementID,
		Lastname:           strings.TrimSpace(in["lastname"]),
		Firstname:          strings.TrimSpace(in["firstname"]),
		Middlename:         strings.TrimSpace(in["middlename"]),
		Birthdate:          strings.TrimSpace(in["birthdate"]),
		Email:              strings.TrimSpace(in["email"]),
		PassportSeries:     strings.TrimSpace(in["passport_series"]),
		PassportNumber:     strings.TrimSpace(in["passport_number"]),
		PassportIssuedBy:   strings.TrimSpace(in["passport_issued_by"]),
		PassportIssuedDate: strings.TrimSpace(in["passport_issued_date"]),
		Address:            strings.TrimSpace(in["address"]),
		PDFFileName:        strings.TrimSpace(in["pdf_filename"]),
	}
	if err := h.repo.CreateApplication(app); err != nil {
		log.Printf("ERR %s db_create agreement_id=%s err=%v", rid, agreementID, err)
		return c.Status(fiber.StatusInternalServerError).JSON(fiber.Map{"ok": false, "error": "db create failed"})
	}
	log.Printf("OK  %s db_create agreement_id=%s", rid, agreementID)

	SetAgreementCookie(c, agreementID)
	log.Printf("INF %s cookie_set agreement_id=%s", rid, agreementID)

	log.Printf("OK  %s step2 dur=%dms", rid, time.Since(start).Milliseconds())
	return c.JSON(fiber.Map{"ok": true, "agreement_id": agreementID})
}

func (h *Handlers) RegisterStep3(c *fiber.Ctx) error {
	if c.Method() != fiber.MethodPost {
		return c.SendStatus(fiber.StatusMethodNotAllowed)
	}
	rid := newRID()
	start := time.Now()

	var in map[string]string
	if err := c.BodyParser(&in); err != nil {
		log.Printf("ERR %s POST /register/step3 bad_json ip=%s sz=%d err=%v", rid, c.IP(), len(c.Body()), err)
		return c.Status(fiber.StatusBadRequest).JSON(fiber.Map{"ok": false, "error": "bad json"})
	}
	log.Printf("REQ %s POST /register/step3 ip=%s sz=%d body=%s", rid, c.IP(), len(c.Body()), dumpMap(in))

	if strings.TrimSpace(in["agree"]) != "1" {
		log.Printf("VAL %s step3 not_agreed", rid)
		return c.Status(fiber.StatusUnprocessableEntity).JSON(fiber.Map{"ok": false, "error": "must agree"})
	}

	log.Printf("OK  %s step3 dur=%dms", rid, time.Since(start).Milliseconds())
	return c.JSON(fiber.Map{"ok": true})
}

func (h *Handlers) Finish(c *fiber.Ctx) error {
	rid := newRID()
	start := time.Now()
	if h.tmpl == nil {
		log.Printf("OK  %s GET /finish dur=%dms (no template)", rid, time.Since(start).Milliseconds())
		return c.SendString("OK")
	}
	var buf bytes.Buffer
	if err := h.tmpl.ExecuteTemplate(&buf, "finish.html", nil); err != nil {
		log.Printf("ERR %s tpl.finish err=%v", rid, err)
		return c.Status(fiber.StatusInternalServerError).SendString("template error")
	}
	log.Printf("OK  %s GET /finish dur=%dms bytes=%d", rid, time.Since(start).Milliseconds(), buf.Len())
	return c.Type("html").Send(buf.Bytes())
}

func (h *Handlers) GetAgreementPDF(c *fiber.Ctx) error {
	rid := newRID()
	start := time.Now()

	id := c.Cookies("agreement_id")
	if id == "" {
		log.Printf("ERR %s GET /agreement missing_cookie ip=%s", rid, c.IP())
		return c.Status(fiber.StatusBadRequest).SendString("missing agreement_id cookie")
	}
	app, err := h.repo.GetByAgreementID(id)
	if err != nil {
		log.Printf("ERR %s db_get agreement_id=%s err=%v", rid, id, err)
		return c.Status(fiber.StatusNotFound).SendString("not found")
	}
	if app == nil {
		log.Printf("ERR %s not_found agreement_id=%s", rid, id)
		return c.Status(fiber.StatusNotFound).SendString("not found")
	}

	path := filepath.Join("data", "agreements", app.PDFFileName)
	log.Printf("INF %s serve_pdf agreement_id=%s file=%s", rid, id, path)
	err = c.SendFile(path)
	if err != nil {
		log.Printf("ERR %s sendfile agreement_id=%s err=%v", rid, id, err)
		return c.Status(fiber.StatusInternalServerError).SendString("sendfile error")
	}
	log.Printf("OK  %s GET /agreement dur=%dms", rid, time.Since(start).Milliseconds())
	return nil
}
