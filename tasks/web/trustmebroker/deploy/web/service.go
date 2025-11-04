package main

import (
	"bytes"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"time"
)

type Service struct {
	repo       Repository
	pdfBaseURL string
}

func NewService(r Repository, pdfBase string) *Service {
	return &Service{repo: r, pdfBaseURL: pdfBase}
}

func GenerateAgreementID() string {
	b := make([]byte, 16)
	_, _ = rand.Read(b)
	return hex.EncodeToString(b)
}

func (s *Service) GeneratePDFForApplication(payload *map[string]string) (string, error) {
	start := time.Now()
	agreementID := (*payload)["agreement_id"]
	filename := (*payload)["pdf_filename"]

	log.Printf("PDF start agreement_id=%s filename=%s base_url=%s", agreementID, filename, s.pdfBaseURL)

	reqBody, err := json.Marshal(payload)
	if err != nil {
		log.Printf("PDF ERR marshal_json agreement_id=%s err=%v", agreementID, err)
		return "", err
	}
	log.Printf("PDF INFO agreement_id=%s json_size=%d body=%s", agreementID, len(reqBody), string(reqBody))

	resp, err := http.Post(s.pdfBaseURL+"/render", "application/json", bytes.NewReader(reqBody))
	if err != nil {
		log.Printf("PDF ERR http_post_fail agreement_id=%s url=%s err=%v", agreementID, s.pdfBaseURL+"/render", err)
		return "", err
	}
	defer resp.Body.Close()

	log.Printf("PDF INFO agreement_id=%s http_status=%d", agreementID, resp.StatusCode)

	if resp.StatusCode != http.StatusOK {
		b, _ := io.ReadAll(resp.Body)
		log.Printf("PDF ERR http_status agreement_id=%s code=%d body=%s", agreementID, resp.StatusCode, string(b))
		return "", &HTTPError{StatusCode: resp.StatusCode, Body: string(b)}
	}

	pdfBytes, err := io.ReadAll(resp.Body)
	if err != nil {
		log.Printf("PDF ERR read_body agreement_id=%s err=%v", agreementID, err)
		return "", err
	}
	log.Printf("PDF OK agreement_id=%s bytes=%d", agreementID, len(pdfBytes))

	outPath := filepath.Join("data", "agreements", filename)
	if err := os.MkdirAll(filepath.Dir(outPath), 0755); err != nil {
		log.Printf("PDF ERR mkdir agreement_id=%s path=%s err=%v", agreementID, outPath, err)
		return "", err
	}

	if err := os.WriteFile(outPath, pdfBytes, 0644); err != nil {
		log.Printf("PDF ERR write_file agreement_id=%s path=%s err=%v", agreementID, outPath, err)
		return "", err
	}
	log.Printf("PDF OK write_file agreement_id=%s path=%s size=%d", agreementID, outPath, len(pdfBytes))

	if err := s.repo.UpdatePDFCreated(filename); err != nil {
		log.Printf("PDF WARN db_update agreement_id=%s file=%s err=%v", agreementID, filename, err)
	} else {
		log.Printf("PDF OK db_update agreement_id=%s file=%s", agreementID, filename)
	}

	log.Printf("PDF done agreement_id=%s dur=%dms", agreementID, time.Since(start).Milliseconds())
	return outPath, nil
}

type HTTPError struct {
	StatusCode int
	Body       string
}

func (e *HTTPError) Error() string { return e.Body }
