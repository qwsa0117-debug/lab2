use std::io::{self, Write};
use std::str::FromStr;

// 원하는 개수의 숫자가 입력될 때까지 다시 입력받는다.
fn read_values<T: FromStr>(prompt: &str, count: usize) -> Vec<T> {
    loop {
        print!("{}", prompt);
        io::stdout().flush().expect("출력 실패");

        let mut input = String::new();
        io::stdin().read_line(&mut input).expect("입력 실패");

        let values: Result<Vec<T>, _> = input
            .split_whitespace()
            .map(|word| word.parse::<T>())
            .collect();

        match values {
            Ok(v) if v.len() == count => return v,
            _ => println!("숫자 {}개를 공백으로 구분해서 입력하세요.", count),
        }
    }
}

// Vec를 이용하여 입력된 크기만큼 행렬을 생성한다.
fn read_matrix(name: &str, rows: usize, cols: usize) -> Vec<Vec<i32>> {
    let mut matrix = Vec::with_capacity(rows);

    println!("행렬 {}의 원소를 한 행씩 입력하세요.", name);

    for i in 0..rows {
        let prompt = format!("{}의 {}행: ", name, i + 1);
        let row = read_values::<i32>(&prompt, cols);
        matrix.push(row);
    }

    matrix
}

fn main() {
    let size_a = read_values::<usize>("A의 행과 열 크기: ", 2);
    let size_b = read_values::<usize>("B의 행과 열 크기: ", 2);

    if size_a.contains(&0) || size_b.contains(&0) {
        println!("행과 열 크기는 1 이상이어야 합니다.");
        return;
    }

    if size_a != size_b {
        println!("두 행렬의 크기가 달라 덧셈할 수 없습니다.");
        return;
    }

    let rows = size_a[0];
    let cols = size_a[1];

    let a = read_matrix("A", rows, cols);
    let b = read_matrix("B", rows, cols);

    // 두 i32 원소의 합을 안전하게 저장하도록 결과는 i64로 만든다.
    let mut result = vec![vec![0_i64; cols]; rows];

    for i in 0..rows {
        for j in 0..cols {
            result[i][j] = i64::from(a[i][j]) + i64::from(b[i][j]);
        }
    }

    println!("행렬 덧셈 결과:");
    for row in &result {
        for value in row {
            print!("{} ", value);
        }
        println!();
    }
}
