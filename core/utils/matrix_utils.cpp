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
#include "matrix_utils.hpp"

#include "string_utils.hpp"
#include "message.hpp"
#include "core/formula/fortran/fortnode.hpp"


namespace {
const int NUM_ORTHORELAX_LOOP = 50;
}

// MCNP式文字列から変換を生成。カンマ区切りで複数の行列の積を生成可能
math::Matrix<4> utils::generateTransformMatrix(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
		const std::string &orgTrclStr)
{
    //mDebug() << "orgTrclStr=" << orgTrclStr;
	if(orgTrclStr.empty()) return math::Matrix<4>();
	std::string trclArgumentStr = utils::dequote(std::make_pair('(', ')'), orgTrclStr);
	auto trclStrVec = utils::splitString(std::make_pair('{', '}'), ",", trclArgumentStr, true);


	// cellに複数trclを適用する場合カンマで区切る。 "10 0 0, "など
	// ここでは順次Tr行列を作成・合成する。
	math::Matrix<4> matrix = math::Matrix<4>::IDENTITY();
	for(auto trclStr: trclStrVec) {
		utils::trim(&trclStr);

		auto tmpMatrix = math::Matrix<4>::IDENTITY();
		if(utils::isArithmetic(trclStr)) {
			// trclが数字ならばTR番号
			size_t trNum = utils::stringTo<size_t>(trclStr);
			if(trMap.find(trNum) != trMap.end()) {
				tmpMatrix = trMap.at(trNum);
			}
		} else {
			// それ以外の場合はTRカード引数文字列
			tmpMatrix = generateSingleTransformMatrix(trclStr, false);
		}

		matrix = matrix*tmpMatrix;
	}
	return matrix;
}

math::Matrix<4> utils::generateSingleTransformMatrix(const std::string &trstr, bool warnCompat)
{
    //mDebug() << "in GenTrMat trstr=" << trstr;
	auto str = trstr;
	utils::trim(&str);
	utils::tolower(&str); // Card生成までにはincludeを処理済みでなければならない。
	bool isDegreeInput = false;
	if(str.front() == '*') {
		isDegreeInput = true;
		str = str.substr(1);
	}
	std::vector<std::string> strVec = utils::splitString(std::make_pair('{', '}'), " ", str, true);

	// TRの最大パラメータ数は13 最小は3、
	if(strVec.size() < 3 || strVec.size() > 13)  {
		throw std::invalid_argument(std::string("Argument size is invalid for Transformation string=")
									+ utils::concat(strVec, ",") + " size =" + std::to_string(strVec.size()));
	}

	// 平行移動3成分と行列成分9個の入力だが、行列には色々パターンがある。j入力も含めて
	/*
	 * 1. 全9要素入力
	 * 2. vector2個(値6個)入力すれば最後の1vectorは外積で生成される。
	 * 3. One vector each way in the matrix (5 values). The component in common must be less than 1.
	 *    MCNP will fill out the matrix by the Eulerian angles scheme.
	 * 4. vector1個(3要素)入力すれば、MCNPは適当な方法で他の2個のvectorを生成する。
	 * 5. 入力しない。このときは単位行列が設定される.
	 * また、0.001radian以上の非直交変換が検出された場合、警告を出す。
	 */
	std::vector<double> argVec;  // 入力のデフォルト値
	if(!isDegreeInput) {
		 argVec = std::vector<double>{0, 0, 0,   1, 0, 0,   0, 1, 0,   0, 0, 1,  1};
	} else {
		argVec = std::vector<double>{0, 0, 0,   0, 90, 90,   90, 0, 90,   90, 90, 0,  1};
	}
	std::vector<std::size_t> indices;  // argVecへ実際に入力されたindex
	/*
     * MCNPでの行列生成方法は以下の５通り
	 * 1．簡単にクリア
	 * 2．対応する。
	 * 3．Eulerian angle schemeがが良くわからない
	 * 4．は仕様がわからないからパス（ソースを読めばわかる?）
	 * 5．簡単にクリア
	 */
	for(std::size_t i = 0; i < strVec.size(); i++) {
		if(strVec.at(i) != "j") {
			argVec.at(i) = fort::eq(strVec.at(i));
			/*
			 * indicesはargVecへ入力されたindex。(デフォルト値ではない所のindex)
			 * これは回転行列部分に入力されたindexを知りたいので、
			 * 最初の0-2の平行移動部分(strVecのindex<4)と最後の入力値Mは記録しない
			 */
			if(i >= 3 && i != 12)indices.push_back(i-3); // ついでにmatrix部分の先頭がindex=0になるようにずらす。
		}
	}
	if(isDegreeInput) {
		// 角度入力の場合該当部分をcos化する。
		for(size_t i = 3; i < argVec.size()-1; ++i) {
			argVec.at(i) = std::cos(math::toRadians(argVec.at(i)));
		}
	}
	std::vector<double> dispArgs(argVec.begin(), argVec.begin()+3);
	std::vector<double> rotArgs(argVec.begin() + 3, argVec.end()-1);

	// ここの時点で1.と5.はクリア。
	// 次に6要素入力の処理。
	if(indices.size() == 6) {
		if(warnCompat) mWarning() << "Six elements input for rotation matrix is not phits compatible";
		// 入力された6要素が3個ずつ連なっており、かつ0or3or6から始まっているかチェック
		if(indices[2] != indices[1]+1 || indices[1] != indices[0]+1
		|| indices[5] != indices[4]+1 || indices[4] != indices[3]+1
		|| indices[0]%3 != 0 || indices[3]%3 != 0){
			std::cerr << "Error! 2 column vectors input in TR card is not implemented yet. " << std::endl;
			std::exit(EXIT_FAILURE);
		}
		math::Vector<3> vec1{rotArgs.at(indices[0]), rotArgs.at(indices[1]), rotArgs.at(indices[2])};
		math::Vector<3> vec2{rotArgs.at(indices[3]), rotArgs.at(indices[4]), rotArgs.at(indices[5])};
		math::Vector<3> vec3 = math::crossProd(vec1, vec2);
		// ここでindices[0-2]= {0, 1, 2}, indices[3-5]= {6, 7, 8} ならvec1×vec2は y方向の逆を向いてしまうので-1倍する
		if(indices[0] == 0 && indices[3] == 6) vec3 = -1*vec3;

		// これでvec1, vec2, vec3を行ベクトルにすれば3軸が直行した座標変換になる。
		std::vector<std::size_t> targetIndices {0, 1, 2, 3, 4, 5, 6, 7, 8};
		for(auto& ind: indices) {
			targetIndices.erase(std::remove(targetIndices.begin(), targetIndices.end(), ind), targetIndices.end());
		}
		assert(targetIndices.size() == 3);
		rotArgs.at(targetIndices.at(0)) = vec3.x();
		rotArgs.at(targetIndices.at(1)) = vec3.y();
		rotArgs.at(targetIndices.at(2)) = vec3.z();

	} else if (indices.size() < 6 && indices.size() > 0) {
		throw std::invalid_argument("Error! Eulerian input scheme are not implimented yet. Input more than 6 elements.");
	}
	/*
	 *  これで入力値がargVecに全て得られた。
	 *  あとはargVecの最後の要素に従ってアフィン変換行列を作成する。
	 */
	// 平行移動行列を作成

	math::Matrix<4> displacement{1, 0, 0, 0,
								 0, 1, 0, 0,
								 0, 0, 1, 0,
                                dispArgs[0], dispArgs[1], dispArgs[2], 1};

    // 回転移動は最後の入力値Mによって他の入力の解釈が変わる。
    /*
     * M=2 argVec[3-5] がそれぞれ、xyz方向の回転.ここに来るまでにcos化されている。
     * M=1 ふつう。
     */
    int lastValue = std::round(argVec.at(12));


    // 回転行列を作成
    math::Matrix<4> rotation;
    if(std::abs(lastValue) == 2) {
        // argVec[3-5] are rotation cosine.
        rotation = math::Matrix<4>::IDENTITY();
        auto rot3 = generateRotationMatrix2(math::Vector<3>{1,0,0}, std::acos(argVec.at(3)))
            *generateRotationMatrix2(math::Vector<3>{0,1,0}, std::acos(argVec.at(4)))
            *generateRotationMatrix2(math::Vector<3>{0,0,1}, std::acos(argVec.at(5)));
        rotation.setRotationMatrix(rot3);
        mDebug() << "rot3====" << rot3;
    } else {
        math::Matrix<4>::array_type rotArray(math::Matrix<4>::IDENTITY().matrix());
        for(std::size_t i = 0; i < 9; i++) {
            rotArray[i/3][i%3] = (std::abs(rotArgs[i]) > math::EPS) ? rotArgs[i] : 0;
        }
        rotation = math::Matrix<4>(rotArray);
    }

	// 回転行列が直交行列でない場合はグラム・シュミット直交化で直交行列化する。
	math::Matrix<3> rotMat = rotation.rotationMatrix();
	if(!rotMat.isOrthogonal()){
		auto orgMat = rotMat;
		math::Matrix<3>::orthonormalize(&rotMat, NUM_ORTHORELAX_LOOP);
		// 行ベクトルが直行していない場合は警告を出す。
		auto v0 = orgMat.rowVector(0), v1 = orgMat.rowVector(1), v2 = orgMat.rowVector(2);
		if(!math::isOrthogonal(v0, v1) || !math::isOrthogonal(v0, v2) || !math::isOrthogonal(v1, v2)) {
			mWarning() << "Transform matrix is not orthogonal.\n original = " << orgMat
						<< ",\n orthonormalized = " << rotMat;
		}
		math::Matrix<4>::array_type arr {{
			{{rotMat(0, 0), rotMat(0, 1), rotMat(0, 2), 0}},
			{{rotMat(1, 0), rotMat(1, 1), rotMat(1, 2), 0}},
			{{rotMat(2, 0), rotMat(2, 1), rotMat(2, 2), 0}},
			{{0,            0,            0,            1}}
		}};
		rotation = math::Matrix<4>(arr);
	}

//	mDebug() << "dispMatrix=" << displacement;
//	mDebug() << "rotMatrix=" << rotation;
	/*
	 *  TRカードの最後のエントリが負の場合
	 *  ・displacementは-1倍して先に適用、原点周り回転はその後
	 *  TRカードの最後のエントリが正の場合
	 *	・原点周り回転が先、displacementはその後そのまま適用
	 *
	 */
	math::Matrix<4> matrix = math::Matrix<4>::IDENTITY();

    if(lastValue < 0) {
		math::Matrix<4>::array_type tmpArray = displacement.matrix();
		tmpArray[3][0] *= -1;
		tmpArray[3][1] *= -1;
		tmpArray[3][2] *= -1;
		displacement = math::Matrix<4>(tmpArray);
        matrix = displacement*rotation;
    } else {
		matrix = rotation*displacement;
	}
//	mDebug() << "TR=" << matrix;
	return matrix;

//	return generateTransformMatrix(std::unordered_map<size_t, math::Matrix<4>>(), orgTrclStr);
}

std::string utils::toTrclString(const math::Matrix<4> &matrix)
{
	std::stringstream ss;
	auto trVec = matrix.translationVector();
	ss << trVec.x() << " " << trVec.y() << " " << trVec.z() << " ";

	for(size_t i = 0; i < 3; ++i) {
		for(size_t j = 0; j < 3; ++j) {
			ss << " " << utils::toString(matrix(i, j));
		}
	}
	return ss.str();
}

math::Matrix<4> utils::generateTransformMatrix(const std::string &trstr)
{
	return generateTransformMatrix(std::unordered_map<size_t, math::Matrix<4>>(), trstr);
}

