package main

import (
	"regexp"
	"strings"
)

func validateStep1(in map[string]string) []string {
	var errs []string
	req := []string{"lastname", "firstname", "birthdate", "email"}
	for _, k := range req {
		if strings.TrimSpace(in[k]) == "" {
			errs = append(errs, k+" is required")
		}
	}
	if e := strings.TrimSpace(in["email"]); e != "" && !isEmailLike(e) {
		errs = append(errs, "email invalid")
	}
	return errs
}

func validateStep2(in map[string]string) []string {
	var errs []string
	req := []string{"passport_series", "passport_number", "passport_issued_by", "passport_issued_date", "address"}
	for _, k := range req {
		if strings.TrimSpace(in[k]) == "" {
			errs = append(errs, k+" is required")
		}
	}
	return errs
}

var emailRe = regexp.MustCompile(`^[a-zA-Z0-9]+@[a-zA-Z0-9]+\.[a-zA-Z]{2,}$`)

func isEmailLike(s string) bool {
	return emailRe.MatchString(s)
}
