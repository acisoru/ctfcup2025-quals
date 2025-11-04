use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, visit::Visit, Expr, ExprCall, StaticMutability};

// Custom visitor to check for function calls, macros, and unsafe blocks
struct Validator {
    errors: Vec<String>,
}

impl Validator {
    fn new() -> Self {
        Self { errors: Vec::new() }
    }

    fn error(&mut self, message: String) {
        self.errors.push(message);
    }

    fn has_errors(&self) -> bool {
        !self.errors.is_empty()
    }
}

impl<'ast> Visit<'ast> for Validator {
    fn visit_item_foreign_mod(&mut self, node: &'ast syn::ItemForeignMod) {
        self.error("External blocks (extern) are not allowed".to_string());
        syn::visit::visit_item_foreign_mod(self, node);
    }

    fn visit_item_fn(&mut self, node: &'ast syn::ItemFn) {
        if let Some(_abi) = &node.sig.abi {
            self.error("External functions are not allowed".to_owned());
        }
        syn::visit::visit_item_fn(self, node);
    }

    fn visit_trait_item_fn(&mut self, node: &'ast syn::TraitItemFn) {
        if let Some(_abi) = &node.sig.abi {
            self.error("External functions in traits are not allowed".to_owned());
        }
        syn::visit::visit_trait_item_fn(self, node);
    }

    fn visit_impl_item_fn(&mut self, node: &'ast syn::ImplItemFn) {
        if let Some(_abi) = &node.sig.abi {
            self.error("External functions in impl blocks (extern {}) are not allowed".to_owned());
        }
        syn::visit::visit_impl_item_fn(self, node);
    }
    fn visit_attribute(&mut self, node: &'ast syn::Attribute) {
        let attribute_name = node.path().get_ident().map(|i| i.to_string());

        let allowed_attributes = ["inline", "warn", "deny", "must_use"];

        if let Some(name) = &attribute_name {
            if !allowed_attributes.contains(&name.as_str()) {
                self.error(format!("Attribute '#[{}]' is not allowed", name));
            }
        } else {
            // Complex paths (like derive, cfg, etc.) are not allowed
            self.error("Complex attributes are not allowed".to_string());
        }

        syn::visit::visit_attribute(self, node);
    }

    fn visit_item_use(&mut self, node: &'ast syn::ItemUse) {
        self.error("Used not allowed".to_owned());
        syn::visit::visit_item_use(self, node);
    }
    fn visit_path_segment(&mut self, node: &'ast syn::PathSegment) {
        let disallowed_patterns = [
            "std",
            "free",
            "malloc",
            "libc",
            "proc_macro",
            "quote",
            "syn",
            "core",
        ];
        let segment = node.ident.to_string();
        if disallowed_patterns.contains(&segment.as_str()) {
            self.error(format!("Disallowed segment '{}'", segment,));
        }
        syn::visit::visit_path_segment(self, node);
    }
    fn visit_expr_call(&mut self, node: &'ast ExprCall) {
        if let Expr::Path(path_expr) = &*node.func {
            let segments = &path_expr.path.segments;

            let disallowed_patterns = [
                "std",
                "free",
                "malloc",
                "libc",
                "proc_macro",
                "quote",
                "syn",
            ];
            for segment in segments.iter() {
                let segment = segment.ident.to_string().to_lowercase();
                if disallowed_patterns.contains(&segment.as_str()) {
                    self.error(format!(
                        "Disallowed segment '{}' in function call '{}'",
                        segment,
                        segments
                            .iter()
                            .map(|s| s.ident.to_string())
                            .collect::<Vec<String>>()
                            .join("::")
                    ));
                }
            }
        }
        syn::visit::visit_expr_call(self, node);
    }

    fn visit_macro(&mut self, node: &'ast syn::Macro) {
        self.error("Macros not allowed".to_owned());
        syn::visit::visit_macro(self, node);
    }

    fn visit_expr_unsafe(&mut self, node: &'ast syn::ExprUnsafe) {
        self.error("Unsafe expressions are not allowed".to_string());
        syn::visit::visit_expr_unsafe(self, node);
    }

    fn visit_foreign_item(&mut self, node: &'ast syn::ForeignItem) {
        self.error("Foreign function interface (FFI) is not allowed".to_string());
        syn::visit::visit_foreign_item(self, node);
    }

    fn visit_item_static(&mut self, node: &'ast syn::ItemStatic) {
        if let StaticMutability::Mut(_) = node.mutability {
            self.error("Mutable statics are not allowed".to_string());
        }
        syn::visit::visit_item_static(self, node);
    }

    fn visit_expr_method_call(&mut self, node: &'ast syn::ExprMethodCall) {
        let func_name = node.method.to_string();

        let disallowed_patterns = ["free", "malloc"];

        if func_name.contains("::") || disallowed_patterns.contains(&func_name.as_str()) {
            self.error(format!("Disallowed method call '{}'", func_name));
        }
        syn::visit::visit_expr_method_call(self, node);
    }

    fn visit_item_impl(&mut self, node: &'ast syn::ItemImpl) {
        if node.unsafety.is_some() {
            self.error("Unsafe impls not allowed".to_string());
        }
        syn::visit::visit_item_impl(self, node)
    }
}

#[proc_macro]
pub fn include_validated(input: TokenStream) -> TokenStream {
    let file_path = parse_macro_input!(input as syn::LitStr);
    let path = file_path.value();
    println!("cargo:rerun-if-changed={}", path);

    let content =
        std::fs::read_to_string(&path).unwrap_or_else(|_| panic!("Failed to read file: {}", path));

    let file =
        syn::parse_file(&content).unwrap_or_else(|_| panic!("Failed to parse file: {}", path));

    let mut validator = Validator::new();

    for item in &file.items {
        validator.visit_item(item);
    }

    if validator.has_errors() {
        let error_msg = validator.errors.join("\n");
        panic!("File validation failed for {}:\n{}", path, error_msg);
    }

    // Return the original file content as tokens
    let tokens = quote!(#file);
    TokenStream::from(tokens)
}
