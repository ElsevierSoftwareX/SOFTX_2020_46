/*!
 * gxsview version 1.2
 *
 * Copyright (c) 2020 Ohnishi Seiki and National Maritime Research Institute, Japan
 *
 * Released under the GPLv3
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * If you need to distribute with another license,
 * ask ohnishi@m.mpat.go.jp
 */
#ifndef EQUATIONSOLVER_HPP
#define EQUATIONSOLVER_HPP

#include <algorithm>
#include <cassert>
#include <complex>
#include <stdexcept>
#include <vector>

#include "constants.hpp"
#include "core/utils/message.hpp"

namespace math {

namespace {
const double INV3 = 0.33333333333333333333333;
// 闇雲にiteration増やしたところで精度は改善しない。
const size_t MAX_ITERATION_FOR4th = 50000;
}


// 複素数数係数2次方程式を解く。
template <class DBLE>
std::vector<std::complex<DBLE>> solve2ndC(std::complex<DBLE> c2, std::complex<DBLE> c1, std::complex<DBLE> c0, bool acceptDoubleRoot)
{
	using comp_type = std::complex<DBLE>;
	// 2次の係数が0なら1次方程式を解く
	if(std::abs(c2) < math::EPS) {
		if(std::abs(c1) < math::EPS) {
			return std::vector<comp_type>();
		} else {
			return std::vector<comp_type>{-c0/c1};
		}
	}

	comp_type discriminant = c1*c1-4.0*c2*c0;
	// 重解
	if(std::abs(discriminant) < math::EPS) {
		if(acceptDoubleRoot) {
			return std::vector<comp_type>{-0.5*c1/c2};
		} else {
			return std::vector<comp_type>();
		}
	// 2解
	} else {
		// 参考：C言語による最新アルゴリズム辞典 by 奥村晴彦
		// 桁落ち防止のため、一方を直接求めた後、他方は解と係数の関係より求める。
		comp_type solution1 = (c1.real() > 0) ? (-c1 - std::sqrt(discriminant))*0.5/c2
										: (-c1 + std::sqrt(discriminant))*0.5/c2;
		return std::vector<comp_type>{solution1, c0/(c2*solution1)};
	}
}

// 実数係数2次方程式を(realの範囲で)解く。
template <class DBLE>
std::vector<DBLE> solve2ndR(DBLE c2, DBLE c1, DBLE c0, bool acceptDoubleRoot)
{
	// 2次の係数が0なら1次方程式を解く
	if(std::abs(c2) < math::EPS) {
		if(std::abs(c1) < math::EPS) {
			return std::vector<DBLE>();
		} else {
			return std::vector<DBLE>{-c0/c1};
		}
	}

	double discriminant = c1*c1-4*c2*c0;
	// 重解
	if(std::abs(discriminant) < math::EPS) {
		if(acceptDoubleRoot) {
			return std::vector<DBLE>{-0.5*c1/c2};
		} else {
			return std::vector<DBLE>();
		}
	// 虚解
	} else if(discriminant < 0) {
		return std::vector<DBLE>();
	// 実2解
	} else {
		double solution1 = (c1 > 0) ? (-c1 - std::sqrt(discriminant))*0.5/c2
									 : (-c1 + std::sqrt(discriminant))*0.5/c2;
		return std::vector<DBLE>{solution1, c0/(c2*solution1)};
	}
}

template <class DBLE>
DBLE cuberoot(DBLE x)
{
	double s, prev;
	int positive;

	if(x == 0) return 0;
	if(x > 0) {
		positive = 1;
	} else {
		positive = 0;
		x= -x;
	}
	s = (x > 1) ? x : 1;
	do {
		prev = s;
		s = (x/(s*s) + 2*s)/3;
	} while (s < prev);
	return (positive) ? prev: -prev;
}

// 実数係数3次方程式を実数の範囲で解く
template<class DBLE>
std::vector<DBLE> solve3rdR(DBLE c3, DBLE c2, DBLE c1, DBLE c0, bool acceptDoubleRoot)
{
//	mDebug() << "solving 3rd order equation, c3=" << c3 << "c2=" << c2 << "c1=" << c1 << "c0=" << c0;
	// c3が0なら2次式を解く
	if(std::abs(c3) < math::EPS) {
		return solve2ndR(c2, c1, c0, acceptDoubleRoot);
	}

	// 以降c3≠0として進める。
	/*
	 * 解の個数は以下の通り
	 * 実解 1個(3重解)
	 * 実解 2個(1+重解)
	 * 実解 3個
	 * 実解 1個 + 複素共役解 2個
	 *
	 *  ※重解を持つ場合は解は必ず実数（実解1個か2個） 虚解は共役なので等しくならないから
	 */

	// カルダノの公式 (by C言語による最新アルゴリズム辞典 奥村晴彦
	DBLE a=c3, b=c2, c=c1, d=c0;
	b /= (3*a);
	c /= a;
	d /= a;
	DBLE p = b*b - c/3;
	DBLE q = (b*(c - 2*b*b) - d)/2;
	a = q*q - p*p*p;
	/*
	 * 重解が、重解か共役複素解になるかは数値誤差で変わる。
	 * 微妙に虚部が出てくるという形になる。故に多少の誤差があっても重解として
	 * 処理する。
	 */
	//mDebug() << "avalue=" << a;
	if(std::abs(a) < 100*math::EPS) {
		// 3重解
		if(std::abs(q) < math::EPS) {
			//mDebug() << "3重解=" <<-b;
			return (acceptDoubleRoot) ? std::vector<DBLE> {-b}
									  : std::vector<DBLE>();
		}
		// 重解
		q = std::pow(q, INV3);
		auto sol1 = 2.0*q - b, sol2 = -q - b;
//		mDebug() << "実解sol1=" << sol1;  //解1
//		mDebug() << "重解sol2=" << sol2;  //重解
		return (acceptDoubleRoot) ? std::vector<DBLE>{sol1, sol2}
								  : std::vector<DBLE>{sol1};
	} else if (a > 0) {
		// 1実解2複素解.→複素解は重解の仲間
		double a3, b3;
		/*
		 * std::powは第一引数が負で、第二引数が整数でない場合は定義域エラーになる。
		 * ので3乗根はニュートン法による数値解 by C言語に寄るアルゴリズム時点 by 奥村晴彦を使う
		 */
//		a3 = (q > 0) ? std::pow(q + std::sqrt(a), INV3)
//					 : std::pow(q - std::sqrt(a), INV3);
		a3 = (q > 0) ? cuberoot(q + std::sqrt(a))
					 : cuberoot(q - std::sqrt(a));

		b3 = p/a3;
		auto sol1 = a3+b3 - b;                         // これが実解
//		auto sol2 = -0.5*(a3 + b3) - b;                // 複素解1の実部
//		auto sol3 = std::abs(a3 - b3)*std::sqrt(3.0)/2;// 複素解2の実部
//		mDebug() << "実解sol1=" << sol1;
//		mDebug() << "複素解実部sol2=" << sol2;
//		mDebug() << "複素解実部sol3=" << sol3;
		return std::vector<DBLE>{sol1};
	} else {
		// 3実解
		a = std::sqrt(p);
		auto t = std::acos(q/(p*a));
		a*= 2;
		auto sol1 = a*cos(t/3) - b;                 //実解1
		auto sol2 = a*cos((t + 2*math::PI)/3) - b;  //実解2
		auto sol3 = a*cos((t + 4*math::PI)/3) - b;  //実解3
		return std::vector<DBLE>{sol1, sol2, sol3};
	}
}
// 4次式の値の計算
template<class DBLE>
constexpr DBLE func4th(DBLE c4, DBLE c3, DBLE c2, DBLE c1, DBLE c0, DBLE x)
{
	return (((c4*x + c3)*x + c2)*x + c1)*x + c0;
}
// 4次式の勾配を計算
template<class DBLE>
constexpr DBLE deriv4th(DBLE c4, DBLE c3, DBLE c2, DBLE c1, DBLE x)
{
	return ((4*c4*x + 3*c3)*x + 2*c2)*x +c1;
//	return 4*c4*x*x*x + 3*c3*x*x* + 2*c2*x + c1;
}

// init初期値をから最近傍の解へと修正する。
template<class DBLE>
DBLE mod4thRbyNewton(DBLE c4, DBLE c3, DBLE c2, DBLE c1, DBLE c0, DBLE init, DBLE prec)
{
	assert(prec>0);
	DBLE x = init, prevX = init, preprevX = init;
	DBLE funcval = func4th(c4, c3, c2, c1, c0, x), diffval = deriv4th(c4, c3, c2, c1, x);
	size_t count = 0;
	while(funcval*funcval > prec*prec*x*x) {  // f(x)/x > prec を基準にする。
		//mDebug() <<"x=" << x << "f(x)=" << funcval << "f'(x)=" << diffval << "delta=" << -funcval/diffval;

		x -= funcval/diffval;
		if((x - prevX)*(x - prevX) < prec*prec) {
			// 定常に達した場合
//			mWarning() << "Converged before prec satisfied, cnts=" << count << "coeff="
//						<< c4 << c3 << c2 << c1 << c0 << "x=" << x << ", init =" << init << "dx=" << x - prevX
//						<< "f(x)=" << funcval << "f'(x)=" << diffval<< "prec=" << prec;
			return x;
		} else if((x - preprevX)*(x - preprevX) < prec*prec) {
			// 振動で平衡に達した場合
			return x;
		} else if(++count > MAX_ITERATION_FOR4th) {
			// 反復最大数を超えた場合
//			mWarning() << "Reached Max iteration in newton method, " << "count=" << count << "coeffs="
//					   << c4 << c3 << c2 << c1 << c0 << "x=" << x << ", init =" << init
//			            << "residual=" << funcval << "prec=" << prec;
			return x;
		} else if(diffval*diffval < math::EPS*math::EPS)  {
			// 勾配が0になった場合
			mWarning() << "Derivative of quartic equation become 0, coeffs="
			           << c4 << c3 << c2 << c1 << c0 << "x=" << x << ", init =" << init;
			return x;
		}
		funcval = func4th(c4, c3, c2, c1, c0, x);
		diffval = deriv4th(c4, c3, c2, c1, x);
		preprevX = prevX;
		prevX = x;
	}
	//mDebug() << "count=" << count;
	return x;
}

// 実数係数4次方程式をrealの範囲で解く
// NOTE 桁落ちによる精度改善が数値計算ハンドブックにある
template <class DBLE>
std::vector<DBLE> solve4thR(DBLE c4, DBLE c3, DBLE c2, DBLE c1, DBLE c0, bool acceptDoubleRoot)
{
	//mDebug() << "c4=" << c4 << ", c3=" << c3 << ", c2=" << c2 << ", c1=" << c1 << ", c0=" << c0;
	// c4が0なら3次式を解く
	if(std::abs(c4) < math::EPS) {
		return solve3rdR(c3, c2, c1, c0, acceptDoubleRoot);
	}

	// c1==c3==0 の場合は複二次式
	if(std::abs(c3) < math::EPS && std::abs(c1) < math::EPS) {
		//mDebug() << "複二次式解法開始 c2=" << c2;
		if(std::abs(c2) < math::EPS && std::abs(c0) < math::EPS) {
			// c2==c0==0の場合は自明な解しか存在しない。当然重解として扱う
			if(acceptDoubleRoot) {
				return std::vector<DBLE>{0};
			} else {
				return std::vector<DBLE>();
			}
		}
		//c4x^4 + c2x^2 + c0 =0
		DBLE disc = c2*c2 - 4*c4*c0;
		//mDebug() << "Disc. of compound quadratic=" << disc << "c2=" << c2;
		if(disc < 0 || (disc == 0 && !acceptDoubleRoot)) {
			// "x^2 ＝ 2種類複素数"となる場合、解は全て複素数
			return std::vector<DBLE>();
		} else if (disc == 0 && acceptDoubleRoot) {
			// x^2 = 1種類の重解の場合、解は2種類
			DBLE solution = -c2/(2*c4);
			if(solution < 0) {
				return std::vector<DBLE>();  // 解は複素数
			} else {
				// この時解は2種類出るがいずれも重解なので、4個の解を生成
				return std::vector<DBLE>{std::sqrt(-c2/(2*c4)), -std::sqrt(-c2/(2*c4))};
			}
		} else {
			// x^2 = 2種類の実解の場合、
			// 解4つ(複素解は返さない)
			std::vector<DBLE> solutions;
			DBLE sol1 = (0.5/c4)*(-c2 + std::sqrt(disc));
			DBLE sol2 = (0.5/c4)*(-c2 - std::sqrt(disc));
			//mDebug() << "sol1, 2=" << sol1 << sol2;
			// sol1, sol2は副二次式の解なので、その根号が4次方程式の解となる。
			if(sol1 > 0) {
				solutions.emplace_back(std::sqrt(sol1));
				solutions.emplace_back(-std::sqrt(sol1));
			} else if (sol1 == 0) {
				if(acceptDoubleRoot) solutions.emplace_back(0);
			}
			if(sol2 > 0) {
				solutions.emplace_back(std::sqrt(sol2));
				solutions.emplace_back(-std::sqrt(sol2));
			}
			return solutions;
		}
	}

	// フェラーリの方法
	const DBLE B3 = 0.25*c3/c4;
	const DBLE p = c2/c4 - 6*B3*B3;
	const DBLE q = c1/c4 - 2*c2/c4*B3 + 8*B3*B3*B3;
	const DBLE r = c0/c4 - c1/c4*B3 + c2/c4*B3*B3 -3*B3*B3*B3*B3;
	//mDebug() << "B3=" << B3 << "p=" << p << "q=" << q << "r=" << r;
	/*
	 * y^4 + py^2 + qy + r =0,  y=x+B3  に変形
	 */
	// q==0の場合は複2次式で解く。実係数ならq=0...とも限らない。
	// 現実的には倍精度でも q は 1e-7程度でしか期待できない…これに安全率10を掛ける
	/*
	 * 実解しか出ないように丸めたところで、結局数値誤差によって交点が
	 * トーラス上から外れるのは避けようがない。
	 */
	if(std::abs(q) < 1e-6) {
		//mDebug() << "複二次式で解く！！！！";
		std::vector<DBLE> ys = solve4thR<DBLE>(1, 0, p, 0, r, acceptDoubleRoot);
		for(auto &y: ys) {
			y -= B3;
		}
		return ys;
	}

	// uは分解方程式 u(p+u)^2 -4ru -q^2 = 0の解
	// u3  +  2p u^2  +  (p^2-4r) u  -  q^2  = 0
	//mDebug() << "cu3=" << 1 << "cu2=" << 2*p << "cu1=" << p*p-4*r << "cu0=" << -q*q;
	std::vector<DBLE> us;
	us = solve3rdR(1.0, 2*p, p*p-4*r, -q*q, true);
	// 実数係数3次方程式なのでuは少なくとも1つの実数解を持つ。= us.front()
	// しかし、解が正であるとは限らない？？その場合sqrt(u)が複素数になる。
	// オイラーの方法を見ると、実係数方程式ならuが正なのは保証されているように見える...
	//mDebug() << "us=" << us;
	DBLE u = us.front();
	u = mod4thRbyNewton<DBLE>(0, 1, 2*p, p*p-4*r, -q*q, u, 1e-12);
	//mDebug() << "修正後のu=" << u;
	if(u >= 0) {
		std::vector<DBLE> y1s, y2s;
		// 方程式1 y^2 -sqrt(u)y +0.5*(p+u)+0.5*q*sqrt(u)/u
		y1s = solve2ndR(1.0, -std::sqrt(u), 0.5*(p+u)+0.5*q*sqrt(u)/u, acceptDoubleRoot);
		//mDebug() << "Disc1=" << -u -4*0.5*(p+u)+0.5*q*sqrt(u)/u;
		// 方程式2 y^2 +sqrt(u)y +0.5*(p+u)-0.5*q*sqrt(u)/u の解
		y2s = solve2ndR(1.0,  std::sqrt(u), 0.5*(p+u)-0.5*q*sqrt(u)/u, acceptDoubleRoot);
		// x=2の4重解ならy2sは解なし、y1sはy=4の重解になるはず
		//mDebug() << "y1s=" << y1s << ", y2s=" << y2s;
		y1s.insert(y1s.end(), y2s.begin(), y2s.end());
		for(auto &y: y1s) {
			y -= B3;
		}
		return y1s;
	} else {
		std::vector<std::complex<DBLE>> y1s, y2s;
		std::complex<DBLE> im(0, 1);
		std::complex<DBLE> cq(q, 0), cp(p, 0);
		y1s = solve2ndC(std::complex<DBLE>(1,0), -im*std::sqrt(-u), 0.5*(cp+u)+0.5*cq*im*sqrt(-u)/u, acceptDoubleRoot);
		y2s = solve2ndC(std::complex<DBLE>(1,0),  im*std::sqrt(-u), 0.5*(cp+u)-0.5*cq*im*sqrt(-u)/u, acceptDoubleRoot);
		y1s.insert(y1s.end(), y2s.begin(), y2s.end());
		std::vector<DBLE> retvec(y1s.size());
		for(size_t i = 0; i < y1s.size(); ++i) {
			retvec.at(i) = y1s.at(i).real() - B3;
		}
		return retvec;
	}
}

// TODO template<class DBLE> solveEq(std::vector<DBLE>, DBLE, bool) みたいにラップすればインターフェイスはきれいになる
//template <class DBLE>
//std::vector<DBLE> solveEqR(std::vector<DBLE>coeffs, DBLE x, bool acceptDoubleRoot)
//{
//	if(coeffs.size() == 2) {
//		return
//	}
//}


}  // end namespace math
#endif // EQUATIONSOLVER_HPP
