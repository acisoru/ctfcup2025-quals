use clap::Parser;
use libc::c_long;
use nix::{
    sys::{ptrace, signal, wait},
    unistd::{fork, ForkResult, Pid},
};
use num::{bigint::ToBigInt, BigInt, BigUint, FromPrimitive, One, Zero};
use std::arch::asm;
use std::fmt::Display;
use std::ops::{Add, Mul, Sub};

const BIGINT_NANOMITES_MUL: u64 = 0x1001;
const BIGINT_NANOMITES_ADD: u64 = 0x1002;
const BIGINT_NANOMITES_SUB: u64 = 0x1003;
const BIGINT_NANOMITES_MODINV: u64 = 0x1004;
const BIGINT_NANOMITES_LEGANDRE: u64 = 0x1005;
const BIGINT_NANOMITES_SQRT: u64 = 0x1006;

fn egcd(a: &BigInt, b: &BigInt) -> (BigInt, BigInt, BigInt) {
    let zero = BigInt::zero();
    let one = BigInt::one();
    let (mut a, mut b) = (a.clone(), b.clone());
    let (mut x0, mut x1, mut y0, mut y1) = (one.clone(), zero.clone(), zero.clone(), one.clone());
    let mut q;
    while !a.is_zero() && !b.is_zero() {
        q = &a / &b;
        let temp_b = b.clone();
        b = &a % &b;
        a = temp_b;

        let temp_x1 = x1.clone();
        x1 = &x0 - &q * &x1;
        x0 = temp_x1;

        let temp_y1 = y1.clone();
        y1 = &y0 - &q * &y1;
        y0 = temp_y1;
    }
    (a, x0, y0)
}

fn modinv(n: &BigUint, p: &BigUint) -> BigUint {
    let n = n.to_bigint().unwrap();
    let p = p.to_bigint().unwrap();
    let (_, kn, _) = egcd(&n, &p);
    ((kn % p.clone() + p.clone()) % p).to_biguint().unwrap()
}

fn read_memory(pid: Pid, addr: usize, length: usize) -> Vec<u8> {
    let mut data = Vec::with_capacity(length);
    let mut offset = 0;
    while offset < length {
        let word = ptrace::read(pid, (addr + offset) as ptrace::AddressType).unwrap();
        let bytes = word.to_ne_bytes();
        for &b in &bytes {
            if data.len() < length {
                data.push(b);
            }
        }
        offset += std::mem::size_of::<c_long>();
    }
    data
}

fn write_memory(pid: Pid, addr: usize, data: &[u8]) {
    let mut offset = 0;
    while offset < data.len() {
        let mut word_bytes = [0u8; std::mem::size_of::<c_long>()];
        let bytes_to_copy = std::cmp::min(std::mem::size_of::<c_long>(), data.len() - offset);
        word_bytes[..bytes_to_copy].copy_from_slice(&data[offset..offset + bytes_to_copy]);
        let word = c_long::from_ne_bytes(word_bytes);
        ptrace::write(pid, (addr + offset) as ptrace::AddressType, word as c_long).unwrap();
        offset += std::mem::size_of::<c_long>();
    }
}

#[derive(Clone, PartialEq, Eq)]
struct MyBigInt {
    value: [u8; 128],
}

impl MyBigInt {
    fn from_u64(n: u64) -> Self {
        let mut value = [0u8; 128];
        value[..8].copy_from_slice(&n.to_le_bytes());
        MyBigInt { value }
    }

    fn from_bytes_le(bytes: &[u8]) -> Self {
        let mut value = [0u8; 128];
        let len = bytes.len().min(128);
        value[..len].copy_from_slice(&bytes[..len]);
        MyBigInt { value }
    }

    fn modinv(&self) -> MyBigInt {
        let result = MyBigInt { value: [0u8; 128] };
        unsafe {
            asm!(
                "mov byte ptr [0], 17",
                in("rax") BIGINT_NANOMITES_MODINV,
                in("rdi") self.value.as_ptr(),
                in("r8") result.value.as_ptr(),
            );
        }
        result
    }

    fn sqrt(&self) -> MyBigInt {
        let result = MyBigInt { value: [0u8; 128] };
        unsafe {
            asm!(
                "mov byte ptr [0], 17",
                in("rax") BIGINT_NANOMITES_SQRT,
                in("rdi") self.value.as_ptr(),
                in("r8") result.value.as_ptr(),
            );
        }
        result
    }

    fn is_quadratic_residue(&self) -> bool {
        let mut result: u64 = 0;
        unsafe {
            asm!(
                "mov byte ptr [0], 17",
                in("rax") BIGINT_NANOMITES_LEGANDRE,
                in("rdi") self.value.as_ptr(),
                out("r8") result,
            );
        }
        result != 0
    }
}

impl Display for MyBigInt {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let bigint = BigUint::from_bytes_le(&self.value);
        write!(f, "{}", bigint)
    }
}

fn nano_op_2(op: u64, a: &MyBigInt, b: &MyBigInt) -> MyBigInt {
    let result = MyBigInt { value: [0u8; 128] };
    unsafe {
        asm!(
            "mov byte ptr [0], 17",
            in("rax") op,
            in("rdi") a.value.as_ptr(),
            in("rsi") b.value.as_ptr(),
            in("r8") result.value.as_ptr(),
        );
    }
    result
}

impl Add for &MyBigInt {
    type Output = MyBigInt;
    fn add(self, other: Self) -> MyBigInt {
        nano_op_2(BIGINT_NANOMITES_ADD, self, other)
    }
}

impl Mul for &MyBigInt {
    type Output = MyBigInt;
    fn mul(self, other: Self) -> MyBigInt {
        nano_op_2(BIGINT_NANOMITES_MUL, self, other)
    }
}

impl Sub for &MyBigInt {
    type Output = MyBigInt;
    fn sub(self, other: Self) -> MyBigInt {
        nano_op_2(BIGINT_NANOMITES_SUB, self, other)
    }
}

impl Add for MyBigInt {
    type Output = MyBigInt;
    fn add(self, other: Self) -> MyBigInt {
        &self + &other
    }
}

impl Mul for MyBigInt {
    type Output = MyBigInt;
    fn mul(self, other: Self) -> MyBigInt {
        &self * &other
    }
}

impl Sub for MyBigInt {
    type Output = MyBigInt;
    fn sub(self, other: Self) -> MyBigInt {
        &self - &other
    }
}

fn handle_bigint_nanomites(pid: Pid) {
    let p = BigUint::from_bytes_le(&[
        11, 144, 179, 20, 130, 181, 54, 70, 237, 218, 34, 233, 155, 77, 139, 217, 159, 94, 230,
        179, 219, 186, 149, 143, 94, 188, 20, 52, 230, 161, 194, 126, 30, 109, 173, 255, 101, 240,
        167, 63, 44, 199, 109, 186, 56, 88, 230, 206, 234, 187, 5, 1, 147, 29, 129, 155, 254, 236,
        48, 26, 236, 254, 1, 30, 234, 137, 91, 249, 143, 25, 59, 249, 135, 53, 48, 30, 213, 159,
        125, 172, 101, 142, 136, 15, 75, 112, 49, 251, 111, 144, 74, 86, 69, 228, 118, 18, 193, 55,
        234, 118, 9, 92, 109, 47, 227, 206, 73, 237, 71, 199, 207, 135, 40, 51, 121, 10, 135, 242,
        221, 193, 27, 240, 32, 123, 203, 10, 28, 158,
    ]);

    // Wait for child to stop after ptrace::traceme()
    let _ = wait::waitpid(pid, None);

    loop {
        match ptrace::cont(pid, None) {
            Ok(_) => {}
            Err(nix::Error::ESRCH) => break,
            Err(_) => {
                break;
            }
        }

        let wait_status = match wait::waitpid(pid, None) {
            Ok(status) => status,
            Err(_) => {
                break;
            }
        };

        match wait_status {
            wait::WaitStatus::Stopped(_, signal::SIGTRAP)
            | wait::WaitStatus::Stopped(_, signal::SIGSEGV) => {
                let mut regs = match ptrace::getregs(pid) {
                    Ok(regs) => regs,
                    Err(_) => {
                        break;
                    }
                };

                match regs.rax {
                    BIGINT_NANOMITES_MODINV => {
                        let n_addr = regs.rdi as usize;
                        let n_bytes = read_memory(pid, n_addr, 128); // Fixed size
                        let n = BigUint::from_bytes_le(&n_bytes);
                        let result = modinv(&n, &p);
                        let mut result_bytes = result.to_bytes_le();
                        result_bytes.resize(128, 0);
                        let result_addr = regs.r8 as usize;
                        write_memory(pid, result_addr, &result_bytes);
                    }
                    BIGINT_NANOMITES_MUL => {
                        let a_addr = regs.rdi as usize;
                        let b_addr = regs.rsi as usize;
                        let a_bytes = read_memory(pid, a_addr, 128);
                        let b_bytes = read_memory(pid, b_addr, 128);
                        let a = BigUint::from_bytes_le(&a_bytes);
                        let b = BigUint::from_bytes_le(&b_bytes);
                        let result = (a * b) % &p;
                        let mut result_bytes = result.to_bytes_le();
                        result_bytes.resize(128, 0);
                        let result_addr = regs.r8 as usize;
                        write_memory(pid, result_addr, &result_bytes);
                    }
                    BIGINT_NANOMITES_ADD => {
                        let a_addr = regs.rdi as usize;
                        let b_addr = regs.rsi as usize;
                        let a_bytes = read_memory(pid, a_addr, 128);
                        let b_bytes = read_memory(pid, b_addr, 128);
                        let a = BigUint::from_bytes_le(&a_bytes);
                        let b = BigUint::from_bytes_le(&b_bytes);
                        let result = (a + b) % &p;
                        let mut result_bytes = result.to_bytes_le();
                        result_bytes.resize(128, 0);
                        let result_addr = regs.r8 as usize;
                        write_memory(pid, result_addr, &result_bytes);
                    }
                    BIGINT_NANOMITES_SUB => {
                        let a_addr = regs.rdi as usize;
                        let b_addr = regs.rsi as usize;
                        let a_bytes = read_memory(pid, a_addr, 128);
                        let b_bytes = read_memory(pid, b_addr, 128);
                        let a = BigUint::from_bytes_le(&a_bytes);
                        let b = BigUint::from_bytes_le(&b_bytes);
                        let result = if a >= b {
                            (a - b) % &p
                        } else {
                            (&p + a - b) % &p
                        };
                        let mut result_bytes = result.to_bytes_le();
                        result_bytes.resize(128, 0);
                        let result_addr = regs.r8 as usize;
                        write_memory(pid, result_addr, &result_bytes);
                    }
                    BIGINT_NANOMITES_LEGANDRE => {
                        let n_addr = regs.rdi as usize;
                        let n_bytes = read_memory(pid, n_addr, 128);
                        let n = BigUint::from_bytes_le(&n_bytes);
                        let exponent = (&p - BigUint::one()) / BigUint::from_u32(2).unwrap();
                        let result = n.modpow(&exponent, &p);
                        regs.r8 = if result == BigUint::one() { 1 } else { 0 };
                        ptrace::setregs(pid, regs).unwrap();
                    }
                    BIGINT_NANOMITES_SQRT => {
                        let n_addr = regs.rdi as usize;
                        let n_bytes = read_memory(pid, n_addr, 128);
                        let n = BigUint::from_bytes_le(&n_bytes);
                        let exponent = (&p + BigUint::one()) / BigUint::from_u32(4).unwrap();
                        let result = n.modpow(&exponent, &p);
                        let mut result_bytes = result.to_bytes_le();
                        result_bytes.resize(128, 0);
                        let result_addr = regs.r8 as usize;
                        write_memory(pid, result_addr, &result_bytes);
                    }
                    _ => {}
                }
                regs.rip += 8;
                ptrace::setregs(pid, regs).unwrap();
            }
            wait::WaitStatus::Exited(_, _) | wait::WaitStatus::Signaled(_, _, _) => break,
            _ => continue,
        }
    }
}

#[derive(Parser)]
struct Cli {
    flag: String,
}

#[derive(Clone, PartialEq, Eq)]
struct AffinePoint {
    x: MyBigInt,
    y: MyBigInt,
}

impl AffinePoint {
    fn new(x: MyBigInt, y: MyBigInt) -> Self {
        AffinePoint { x, y }
    }
}

#[derive(Clone)]
struct JacobianPoint {
    x: MyBigInt,
    y: MyBigInt,
    z: MyBigInt,
}

impl JacobianPoint {
    fn new(x: MyBigInt, y: MyBigInt, z: MyBigInt) -> Self {
        JacobianPoint { x, y, z }
    }

    fn to_affine(&self) -> AffinePoint {
        if self.is_infinity() {
            panic!("kek");
        }
        let z_inv = self.z.modinv();
        let z_inv2 = &z_inv * &z_inv;
        let z_inv3 = &z_inv2 * &z_inv;
        let x_affine = &self.x * &z_inv2;
        let y_affine = &self.y * &z_inv3;
        AffinePoint::new(x_affine, y_affine)
    }

    fn from_affine(p: AffinePoint) -> Self {
        JacobianPoint::new(p.x, p.y, MyBigInt::from_u64(1))
    }

    fn is_infinity(&self) -> bool {
        self.z == MyBigInt::from_u64(0)
    }

    fn infinity() -> Self {
        JacobianPoint::new(
            MyBigInt::from_u64(1),
            MyBigInt::from_u64(1),
            MyBigInt::from_u64(0),
        )
    }

    fn add(&self, other: &JacobianPoint) -> JacobianPoint {
        if self.is_infinity() {
            return other.clone();
        }
        if other.is_infinity() {
            return self.clone();
        }

        let z1z1 = &self.z * &self.z;
        let z2z2 = &other.z * &other.z;

        let u1 = &self.x * &z2z2;
        let u2 = &other.x * &z1z1;

        let s1 = &self.y * &other.z;
        let s1 = &s1 * &z2z2;

        let s2 = &other.y * &self.z;
        let s2 = &s2 * &z1z1;

        if u1 == u2 {
            if s1 == s2 {
                return self.double();
            } else {
                return JacobianPoint::infinity();
            }
        }

        let h = &u2 - &u1;
        let i = &h + &h;
        let i = &i * &i;
        let j = &h * &i;

        let r = &s2 - &s1;
        let r = &r + &r; // r = 2*(s2 - s1)

        let v = &u1 * &i;

        let x3 = &r * &r;
        let x3 = &x3 - &j;
        let x3 = &x3 - &v;
        let x3 = &x3 - &v;

        let y3 = r * (&v - &x3) - (&s1 * &j);
        let y3 = &y3 - &(&s1 * &j); // y3 = r*(v - x3) - 2*s1*j

        let z3 = (&self.z + &other.z) * (&self.z + &other.z) - z1z1 - z2z2;
        let z3 = &z3 * &h;

        JacobianPoint::new(x3, y3, z3)
    }

    fn double(&self) -> JacobianPoint {
        if self.is_infinity() {
            return JacobianPoint::infinity();
        }

        let a = MyBigInt::from_u64(1337); // Curve parameter a

        let xx = &self.x * &self.x;
        let yy = &self.y * &self.y;
        let yyyy = &yy * &yy;
        let zz = &self.z * &self.z;
        let zzzz = &zz * &zz;

        // s = 4*x*y^2
        let s = &self.x * &yy;
        let s = &s + &s; // 2*x*y^2
        let s = &s + &s; // 4*x*y^2

        // m = 3*x^2 + a*z^4
        let m_xx = &xx + &xx + xx; // 3*x^2
        let az4 = &a * &zzzz;
        let m = &m_xx + &az4;

        // x' = m^2 - 2*s
        let x3 = &m * &m;
        let x3 = &x3 - &s;
        let x3 = &x3 - &s;

        // y' = m*(s - x') - 8*y^4
        let y4 = &yyyy + &yyyy; // 2*y^4
        let y4 = &y4 + &y4; // 4*y^4
        let y4 = &y4 + &y4; // 8*y^4

        let y3 = m * (s - x3.clone()) - y4;

        // z' = 2*y*z
        let z3 = &self.y * &self.z;
        let z3 = &z3 + &z3;

        JacobianPoint::new(x3, y3, z3)
    }

    fn multiply(&self, scalar: u64) -> JacobianPoint {
        let mut scalar = scalar;
        let mut result = JacobianPoint::infinity();
        let mut base = self.clone();

        while scalar != 0 {
            if scalar % 2 == 1 {
                result = result.add(&base);
            }
            base = base.double();
            scalar /= 2;
        }

        result
    }
}

fn child_main() {
    let args = Cli::parse();
    let x = MyBigInt::from_bytes_le(args.flag.as_bytes());
    let a = MyBigInt::from_u64(1337);
    let x2 = &x * &x;
    let x3 = &x2 * &x;
    let ax = &a * &x;
    let y2 = &x3 + &ax;
    if !y2.is_quadratic_residue() {
        println!("no");
        return;
    }
    let y = y2.sqrt();
    let target = AffinePoint {
        x: MyBigInt::from_bytes_le(&[
            98, 14, 104, 136, 98, 41, 45, 58, 226, 50, 168, 75, 227, 252, 244, 67, 254, 76, 227,
            132, 217, 79, 141, 53, 170, 26, 36, 251, 28, 145, 43, 161, 212, 165, 24, 20, 18, 99,
            90, 143, 142, 60, 141, 12, 164, 161, 77, 103, 92, 202, 150, 80, 233, 127, 130, 112,
            191, 129, 163, 33, 162, 219, 142, 49, 20, 85, 225, 39, 49, 233, 187, 135, 45, 98, 18,
            60, 52, 220, 111, 3, 115, 128, 255, 70, 144, 144, 213, 143, 187, 172, 212, 176, 180,
            174, 217, 126, 26, 64, 140, 129, 136, 152, 52, 105, 225, 177, 249, 140, 184, 92, 62,
            83, 138, 228, 38, 13, 163, 244, 221, 123, 82, 254, 59, 34, 112, 131, 102, 126,
        ]),
        y: MyBigInt::from_bytes_le(&[
            50, 4, 211, 159, 208, 216, 152, 29, 106, 119, 140, 199, 160, 11, 52, 179, 27, 1, 232,
            189, 254, 23, 70, 133, 171, 175, 94, 145, 20, 47, 5, 190, 161, 216, 182, 117, 63, 108,
            161, 4, 254, 149, 241, 185, 225, 158, 49, 188, 240, 23, 100, 161, 149, 78, 28, 62, 229,
            206, 162, 148, 159, 107, 109, 24, 194, 152, 214, 71, 222, 9, 43, 134, 217, 197, 138, 2,
            12, 151, 84, 5, 109, 48, 140, 138, 101, 228, 88, 192, 243, 242, 182, 255, 119, 165, 67,
            230, 67, 20, 144, 178, 164, 14, 181, 17, 105, 231, 136, 36, 15, 189, 213, 95, 1, 165,
            133, 252, 87, 176, 56, 59, 93, 69, 8, 124, 88, 76, 192, 156,
        ]),
    };
    let point = JacobianPoint::from_affine(AffinePoint { x, y })
        .multiply(31337)
        .to_affine();

    if point == target {
        println!("yes")
    } else {
        println!("no")
    }
}

fn main() {
    match unsafe { fork() } {
        Ok(ForkResult::Parent { child, .. }) => {
            handle_bigint_nanomites(child);
        }
        Ok(ForkResult::Child) => {
            ptrace::traceme().unwrap();
            child_main();
        }
        Err(_) => {}
    }
}
