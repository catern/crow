(define false 0)
(define true 1)
(define fact (lambda (x) (if (= x 1) 1 (* x (fact (- x 1))))))

(define (fact x) (define (iter y) (if (= y 1) 1 (* y (fact (- y 1))))) (iter x))

(define length (lambda (pair) (if (null? (cdr pair)) 
                                1 
                                (+ 1 (length (cdr pair))))))
(define (deriv g) (lambda (x) (/ (- (g (+ x dx)) (g x)) dx))) 
(define (cube x) (* x x x))
(define (square x) (* x x))
(define (fib n) (cond ((= n 0) 0) ((= n 1) 1) (else (+ (fib (- n 1)) (fib (- n 2))))))
nil
