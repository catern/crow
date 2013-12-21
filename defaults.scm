(define fact (lambda (x) (if (= x 1) 1 (* x (fact (- x 1))))))
(define false 0)
(define true 1)

(define force (lambda (x) (x)))

(define streamcar (lambda (x) (car x)))
(define streamcdr (lambda (x) (force (cdr x))))

(define (fact x) (define (iter y) (if (= y 1) 1 (* y (fact (- y 1))))) (iter x))

(define integers (lambda (n) (cons n (delay (integers (+ n 1))))))

(define streammap (lambda (proc list) (cons (proc (streamcar list)) (delay (streammap proc (streamcdr list))))))

(define int-seq (lambda (n) (if (= n 0) nil (cons n (int-seq (- n 1))))))
(define nums (int-seq 5))
(define map (lambda (proc list) (if (null? list) nil (cons (proc (car list)) (map proc (cdr list))))))

(define list (lambda (. x) (begin x)))
(define newline (lambda () (display '\n)))
(define error (lambda (msg value) 
                (begin (display msg) (display value) (newline))))

(define length (lambda (pair) (if (null? (cdr pair)) 
                                1 
                                (+ 1 (length (cdr pair))))))

(define cadr (lambda (pair) (car (cdr pair))))
(define caddr (lambda (pair) (car (cdr (cdr pair)))))
(define cadddr (lambda (pair) (car (cdr (cdr (cdr pair))))))
(define caadr (lambda (pair) (car (car (cdr pair)))))
(define cdadr (lambda (pair) (cdr (car (cdr pair)))))
(define cddr (lambda (pair) (cdr (cdr pair))))
(define cdddr (lambda (pair) (cdr (cdr (cdr pair)))))
(define (not x) (if x false true))

(define (deriv g) (lambda (x) (/ (- (g (+ x dx)) (g x)) dx))) 
(define (cube x) (* x x x))
(define (square x) (* x x))
(define (newton-transform g) (lambda (x) (- x (/ (g x) ((deriv g) x))))) 
(define dx 0.0001) 
(define tolerance .01)
(define (fixed-point f first-guess) (define (close-enough? v1 v2) (< (abs (- v1 v2)) tolerance)) (define (try guess) (let ((next (f guess))) (if (close-enough? guess next) next (try next)))) (try first-guess)) 
(define (average x y) (/ (+ x y) 2)) 
(define (newtons-method g guess) (fixed-point (newton-transform g) guess)) 
(define (sqrt x) (newtons-method (lambda (y) (- (square y) x)) 1.0)) 
(define (fib n) (cond ((= n 0) 0) ((= n 1) 1) (else (+ (fib (- n 1)) (fib (- n 2))))))
nil
