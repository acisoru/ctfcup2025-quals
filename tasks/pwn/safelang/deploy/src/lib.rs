use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, visit::Visit, Expr, ExprCall, ExprMacro, StaticMutability};

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
    fn visit_expr_call(&mut self, node: &'ast ExprCall) {
        if let Expr::Path(path_expr) = &*node.func {
            let segments = &path_expr.path.segments;

            let disallowed_patterns = ["std", "free", "malloc", "libc"];
            for segment in segments.iter() {
                let segment = segment.ident.to_string();
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

    fn visit_expr_macro(&mut self, node: &'ast ExprMacro) {
        let macro_name = node
            .mac
            .path
            .get_ident()
            .map(|ident| ident.to_string())
            .unwrap_or_else(|| "unknown".to_string());

        let allowed_macros = ["vec", "print", "println"];

        if !allowed_macros.contains(&macro_name.as_str()) {
            self.error(format!("Disallowed macro '{}!'", macro_name));
        }

        syn::visit::visit_expr_macro(self, node);
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
