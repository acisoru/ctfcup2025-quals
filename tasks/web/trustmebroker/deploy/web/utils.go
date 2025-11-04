package main

import (
	"math/rand"
	"strings"
	"time"

	"github.com/gofiber/fiber/v2"
)

func newAgreementID() string {
	const alphabet = "abcdefghijklmnopqrstuvwxyz0123456789"
	rand.Seed(time.Now().UnixNano())
	b := make([]byte, 16)
	for i := range b {
		b[i] = alphabet[rand.Intn(len(alphabet))]
	}
	return string(b)
}

func sanitize(a string) string {
	a = strings.ReplaceAll(a, "<", "")
	a = strings.ReplaceAll(a, ">", "")
	a = strings.ReplaceAll(a, "\"", "")
	a = strings.ReplaceAll(a, "'", "")
	a = strings.ReplaceAll(a, "\\", "")
	cut := 17
	if len(a) < 17 {
		cut = len(a)
	}
	a = a[:cut]
	return a
}

func SetAgreementCookie(c *fiber.Ctx, agreementID string) {
	c.Cookie(&fiber.Cookie{
		Name:     "agreement_id",
		Value:    agreementID,
		Path:     "/",
		Expires:  time.Now().Add(1 * time.Hour),
		HTTPOnly: true,
		SameSite: "Strict",
	})
}
