/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configTest.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/20 14:36:56 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/31 22:06:22 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"

int main() {
  std::cout << "==test1: normal file==" << std::endl;
  Config config("test.conf"); // 정상 conf
  std::cout << "==test2: no file==" << std::endl;
  Config config2("nofile.conf"); // 없는 conf
  std::cout << "==test3: trublesome file==" << std::endl;
  Config config3("test2.conf"); // 문제있는 conf
}
